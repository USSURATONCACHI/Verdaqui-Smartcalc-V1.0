#include "calculator.h"

#include <limits.h>

#include "../util/allocator.h"
#include "native_functions.h"






static bool cb_context_is_function(const CalcBackend* this, StrSlice name) {
  const char* const native_functions[] = NATIVE_FUNCTION_NAMES;
  for (int i = 0; i < (int)LEN(native_functions); i++) {
    if (str_slice_eq_ccp(name, native_functions[i])) return true;
  }

  for (int i = 0; i < this->expressions.length; i++) {
    CalcExpr* item = &this->expressions.data[i];
    if (item->type is CALC_EXPR_FUNCTION and
        str_slice_eq_ccp(name, item->function.name.string))
      return true;
  }

  if (this->parent)
    return cb_context_is_function(this->parent, name);
  else
    return false;
}

static ExprValueResult cb_context_get_variable(const CalcBackend* this,
                                               StrSlice name) {
  debug("Request for variable: %.*s\n", name.length, name.start);
  for (int i = 0; i < this->values.length; i++) {
    CalcValue* item = &this->values.data[i];
    if (str_slice_eq_ccp(name, item->name.string))
      return (ExprValueResult){
          .is_ok = true,
          .ok = expr_value_clone(&item->value),
      };
  }
  debugln("Value not found, need to compute");

  for (int i = 0; i < this->expressions.length; i++) {
    CalcExpr* item = &this->expressions.data[i];
    if (item->type is CALC_EXPR_VARIABLE and
        str_slice_eq_ccp(name, item->variable_name.string) is true)
      return expr_calculate(&item->expression,
                                calc_backend_get_context(this));
  }

  if (this->parent)
    return cb_context_get_variable(this->parent, name);
  else
    return (ExprValueResult){
        .is_ok = false,
        .err_pos = null,
        .err_text =
            str_owned("Variable '%.*s' is not found", name.length, name.start),
    };
}

static vec_ExprValue clone_vec_ExprValue(const vec_ExprValue* source) {
  vec_ExprValue result = vec_ExprValue_with_capacity(source->length);

  for (int i = 0; i < source->length; i++)
    vec_ExprValue_push(&result, expr_value_clone(&source->data[i]));

  return result;
}

static ExprValueResult cb_context_call_function(CalcBackend* this,
                                                StrSlice function,
                                                vec_ExprValue* args) {
  // debugln("Request for function: %.*s", function.length, function.start);
  debugln("Calling a function %.*s with %d arguments!", function.length,
          function.start, args->length);

  const NativeFnPtr native_fn = calculator_get_native_function(function);
  if (native_fn) {
    return native_fn(clone_vec_ExprValue(args));
  }

  debugln("It is not native function");

  str_t fn_name = str_slice_to_owned(function);
  CalcExpr* fn_calc_expr = calc_backend_get_function(this, fn_name.string);
  str_free(fn_name);

  debugln("Got CalcExpr* for the function: %p", fn_calc_expr);
  if (fn_calc_expr) {
    debug("Its expr: '");
    expr_print(&fn_calc_expr->expression, DEBUG_OUT);
    debugc("'\n");
  }

  ExprValueResult result;
  if (fn_calc_expr) {
    CalcBackend local_ctx = {
        .parent = this,
        .expressions = vec_CalcExpr_create(),
        .values = vec_CalcValue_create(),
    };

    int i;
    for (i = 0; i < args->length; i++) {
      str_t* name = &fn_calc_expr->function.args.data[i];
      CalcValue val = {
          .name = i < fn_calc_expr->function.args.length
                      ? str_literal(name->string)
                      : str_literal(""),
          .value = expr_value_clone(&args->data[i]),
      };
      debug("Adding an argument '%s' = ", val.name.string);
      expr_value_print(&val.value, DEBUG_OUT);
      debugc("\n");
      vec_CalcValue_push(&local_ctx.values, val);
    }

    for (i = i; i < fn_calc_expr->function.args.length; i++) {
      CalcValue val = {
          .name = str_borrow(&fn_calc_expr->function.args.data[i]),
          .value = {.type = EXPR_VALUE_NONE},
      };
      debug("Adding an empty arg '%s' = ", val.name.string);
      vec_CalcValue_push(&local_ctx.values, val);
    }

    result = expr_calculate_val(&fn_calc_expr->expression,
                                calc_backend_get_context(&local_ctx));

    calc_backend_free(local_ctx);
  } else {
    result = (ExprValueResult){
        .is_ok = false,
        .err_pos = null,
        .err_text =
            str_owned("Function '%.*s' is not found (this shoudn't happen btw)",
                      function.length, function.start),
    };
  }

  return result;
}

ExprContext calc_backend_get_context(const CalcBackend* this) {
  ExprContext result = {
      .context_data = (CalcBackend*)this,
      .call_function = (void*)cb_context_call_function,
      .get_variable = (void*)cb_context_get_variable,
      .is_function = (void*)cb_context_is_function,
  };
  return result;
}

static bool is_unknown_ident(CalcBackend* this, StrSlice text) {
  const char* const native_idents[] = {"cos",  "sin",  "tan",  "acos",
                                       "asin", "atan", "sqrt", "ln",
                                       "log",  "x",    "y"};
  for (int i = 0; i < (int)LEN(native_idents); i++) {
    if (str_slice_eq_ccp(text, native_idents[i])) return false;
  }

  for (int i = 0; i < this->values.length; i++)
    if (str_slice_eq_ccp(text, this->values.data[i].name.string)) return false;

  for (int i = 0; i < this->expressions.length; i++) {
    CalcExpr* item = &this->expressions.data[i];
    if (item->type is CALC_EXPR_VARIABLE) {
      if (str_slice_eq_ccp(text, item->variable_name.string)) return false;
    } else if (item->type is CALC_EXPR_FUNCTION) {
      if (str_slice_eq_ccp(text, item->function.name.string)) return false;
    }
  }

  if (this->parent)
    return is_unknown_ident(this->parent, text);
  else
    return true;
}

static bool is_unknown_ident_tree(CalcBackend* this, TokenTree* tree) {
  if (not tree->is_token) {
    if (tree->data.tree.length is 1)
      return is_unknown_ident_tree(this, &tree->data.tree.data[0]);
    else
      return false;
  }

  if (tree->data.token.type != TOKEN_IDENT) return false;

  return is_unknown_ident(this, tree->data.token.data.ident_text);
}

static bool is_ident_tree(TokenTree* tree) {
  if (not tree->is_token) {
    if (tree->data.tree.length is 1)
      return is_ident_tree(&tree->data.tree.data[0]);
    else
      return false;
  }

  if (tree->data.token.type != TOKEN_IDENT) return false;

  return true;
}

static bool is_action_operator(StrSlice op_text) {
  return str_slice_eq_ccp(op_text, ":=") or str_slice_eq_ccp(op_text, "+=") or
         str_slice_eq_ccp(op_text, "-=") or str_slice_eq_ccp(op_text, "*=") or
         str_slice_eq_ccp(op_text, "/=") or str_slice_eq_ccp(op_text, "%=") or
         str_slice_eq_ccp(op_text, "^=");
}

static bool is_eq_operator(StrSlice op_text) {
  return str_slice_eq_ccp(op_text, "=") or str_slice_eq_ccp(op_text, "==");
}

static bool is_ident(TokenTree* tree) {
  return tree->is_token and tree->data.token.type is TOKEN_IDENT;
}

static bool only_idents(TokenTree* tree) {
  if (tree->is_token) {
    return tree->data.token.type is TOKEN_IDENT;
  }

  int sequence_length = 0;
  for (int i = 0; i < tree->data.tree.length; i++) {
    TokenTree* item = &tree->data.tree.data[i];
    sequence_length++;

    if (tt_token_type(item) is TOKEN_COMMA) {
      if (sequence_length == 0) return false;

      sequence_length = 0;
      continue;
    }

    if (sequence_length == 1) {
      if (not is_ident_tree(item)) return false;
    } else {
      return false;
    }
  }

  return true;
}

static StrSlice collect_variable_name(TokenTree* tree) {
  if (not tree->is_token) {
    if (tree->data.tree.length is 1)
      return collect_variable_name(&tree->data.tree.data[0]);
    else
      panic(
          "Failed to collect variable name (a check of var name is asserted "
          "before this call)");
  }

  if (tree->data.token.type != TOKEN_IDENT)
    panic(
        "Failed to collect variable name (a check of var name is asserted "
        "before this call)");

  return tree->data.token.data.ident_text;
}

static str_t collect_variable(CalcBackend* this, TokenTree tree,
                              int last_eq_pos) {
  TokenTree* first_subtree = &tree.data.tree.data[0];

  StrSlice var_name_slice = collect_variable_name(first_subtree);
  str_t var_name =
      str_owned("%.*s", var_name_slice.length, var_name_slice.start);
  vec_TokenTree var_value = vec_TokenTree_create();

  for (int i = last_eq_pos + 1; i < tree.data.tree.length; i++) {
    debug("Collected: '");
    token_tree_print(&tree.data.tree.data[i], DEBUG_OUT);
    debugc("'\n");
    vec_TokenTree_push(&var_value, tree.data.tree.data[i]);
  }
  char bracket = tree.data.tree_bracket;
  tree.data.tree.length = tree.data.tree.length > last_eq_pos + 1
                              ? last_eq_pos + 1
                              : tree.data.tree.length;  // Extracted elements
  token_tree_free(tree);

  debugln("Collecting vatiable '%s' with bracket '%c'", var_name.string,
          bracket);
  ExprResult res =
      expr_parse_tokens(var_value, bracket, calc_backend_get_context(this));

  if (not res.is_ok) return res.err_text;
  Expr value = res.ok;

  CalcExpr to_add = {
      .type = CALC_EXPR_VARIABLE,
      .variable_name = var_name,
      .expression = value,
  };
  vec_CalcExpr_push(&this->expressions, to_add);
  return str_literal("Ok (variable)");
}

static str_t collect_plot_expr(CalcBackend* this, TokenTree tree) {
  vec_TokenTree tokens;

  char bracket = '{';
  if (tree.is_token) {
    debugln("Plotexpr - token");
    tokens = vec_TokenTree_create();
    vec_TokenTree_push(&tokens, tree);
  } else {
    debugln("Plotexpr - tree");
    tokens = tree.data.tree;
    bracket = tree.data.tree_bracket;
    forget(tree);
  }

  debugln("Plotexpr - Parsing tokens...");
  my_allocator_dump_short();
  ExprResult res =
      expr_parse_tokens(tokens, bracket, calc_backend_get_context(this));
  debugln("Parsed!");

  debugln("Jus wahnnha stoph ere");
  my_allocator_dump_short();

  if (not res.is_ok) return res.err_text;
  Expr value = res.ok;

  CalcExpr to_add = {
      .type = CALC_EXPR_PLOT,
      .expression = value,
  };
  vec_CalcExpr_push(&this->expressions, to_add);
  return str_literal("Ok (plot)");
}

typedef struct FunctionContext {
  CalcBackend* backend;
  vec_str_t variables;
  vec_str_t functions;
} FunctionContext;

bool function_context_is_function(FunctionContext* this,
                                  StrSlice function_name) {
  for (int i = 0; i < this->functions.length; i++) {
    if (str_slice_eq_ccp(function_name, this->functions.data[i].string)) {
      return true;
    }
  }
  ExprContext ctx = calc_backend_get_context(this->backend);
  return ctx.is_function(ctx.context_data, function_name);
}

static void function_context_free(FunctionContext ctx) {
  vec_str_t_free(ctx.variables);
  vec_str_t_free(ctx.functions);
}

struct three_vecs_str_t {
  vec_str_t a;
  vec_str_t b;
  vec_str_t c;
};
static struct three_vecs_str_t collect_function_args(TokenTree args_tt);

typedef struct deconstructed_function_tree {
  TokenTree args_tt;
  str_t name;
  TokenTree right_part;
} DFT;

static DFT deconstruct_function_tree(TokenTree tree) {
  assert_m(not tree.is_token);
  assert_m(tree.data.tree.length is 3);
  TokenTree right_part = vec_TokenTree_popget(&tree.data.tree);
  token_tree_free(vec_TokenTree_popget(&tree.data.tree));  // equality sign
  TokenTree left_part = vec_TokenTree_popget(&tree.data.tree);
  token_tree_free(tree);

  assert_m(not left_part.is_token and left_part.data.tree.length is 2);
  TokenTree args_tt = vec_TokenTree_popget(&left_part.data.tree);
  TokenTree name_tt = vec_TokenTree_popget(&left_part.data.tree);
  token_tree_free(left_part);

  // Collect name
  assert_m(tt_token_type(&name_tt) is TOKEN_IDENT);
  StrSlice name_slice = name_tt.data.token.data.ident_text;
  str_t name = str_slice_to_owned(name_slice);
  token_tree_free(name_tt);

  return (DFT){
      .args_tt = args_tt,
      .name = name,
      .right_part = right_part,
  };
}

static str_t collect_function(CalcBackend* this, TokenTree tree,
                              int last_eq_pos) {
  assert_m(not tree.is_token);
  assert_m(last_eq_pos is 1);
  // Deconstruct
  DFT components = deconstruct_function_tree(tree);
  TokenTree args_tt = components.args_tt, right_part = components.right_part;
  str_t name = components.name;

  struct three_vecs_str_t tmp = collect_function_args(args_tt);
  vec_str_t all_args = tmp.a, variables = tmp.b, functions = tmp.c;

  FunctionContext parse_ctx = {
      .backend = this, .functions = functions, .variables = variables};
  ExprContext ctx = {
      .context_data = &parse_ctx,
      .call_function = null,
      .get_variable = null,
      .is_function = (void*)function_context_is_function,
  };

  str_t result = str_literal("Ok (function)");

  debugln("Parsing right part");
  ExprResult parse_result = expr_parse_token_tree(right_part, ctx);
  debugln("Done parsing right part");
  debugln("Success? %s", parse_result.is_ok ? "true" : "false");

  if (parse_result.is_ok) {
    CalcExpr to_add = {
        .type = CALC_EXPR_FUNCTION,
        .expression = parse_result.ok,
        .function.args = all_args,
        .function.name = name,
    };
    vec_CalcExpr_push(&this->expressions, to_add);
  } else {
    str_free(name);
    vec_str_t_free(all_args);
    str_free(result);
    result = parse_result.err_text;
  }

  function_context_free(parse_ctx);

  return result;
}

static void collect_function_single_arg(TokenTree* arg, vec_str_t* all_args,
                                        vec_str_t* functions,
                                        vec_str_t* variables) {
  if (not arg->is_token) {
    if (arg->data.tree.length is 1) {
      return collect_function_single_arg(&arg->data.tree.data[0], all_args,
                                         functions, variables);
    } else {
      panic("Failed to collect function argument");
    }
  }

  TokenTree* first_token = arg->is_token ? arg : &arg->data.tree.data[0];

  assert_m(first_token->is_token and
           first_token->data.token.type is TOKEN_IDENT);
  bool is_function = not arg->is_token and arg->data.tree.length > 1;

  StrSlice text_slice = first_token->data.token.data.ident_text;
  str_t arg_name = str_slice_to_owned(text_slice);
  vec_str_t_push(all_args, str_clone(&arg_name));

  if (is_function)
    vec_str_t_push(functions, arg_name);
  else
    vec_str_t_push(variables, arg_name);
}

static void collect_function_args_from_vec(vec_TokenTree current_arg,
                                           vec_str_t* all_args,
                                           vec_str_t* functions,
                                           vec_str_t* variables) {
  TokenTree arg_tt = {
      .is_token = false,
      .data.tree_bracket = '<',
      .data.tree = current_arg,
  };

  collect_function_single_arg(&arg_tt, all_args, functions, variables);

  token_tree_free(arg_tt);
}

static struct three_vecs_str_t collect_function_args(TokenTree args_tt) {
  vec_str_t all_args = vec_str_t_create();
  vec_str_t variables = vec_str_t_create();
  vec_str_t functions = vec_str_t_create();

  if (args_tt.is_token) {
    collect_function_single_arg(&args_tt, &all_args, &functions, &variables);
  } else {
    vec_TokenTree current_arg = vec_TokenTree_create();
    for (int i = 0; i < args_tt.data.tree.length; i++) {
      TokenTree* arg = &args_tt.data.tree.data[i];

      if (arg->is_token and arg->data.token.type is TOKEN_COMMA) {
        collect_function_args_from_vec(current_arg, &all_args, &functions,
                                       &variables);
        current_arg = vec_TokenTree_create();
      } else {
        vec_TokenTree_push(&current_arg, *arg);
      }
    }
    args_tt.data.tree.length = 0;  // Extracted elements

    if (current_arg.length > 0)
      collect_function_args_from_vec(current_arg, &all_args, &functions,
                                     &variables);
    else
      vec_TokenTree_free(current_arg);
  }
  token_tree_free(args_tt);

  return (struct three_vecs_str_t){all_args, variables, functions};
}

static void find_equalities_and_actions(TokenTree* tree, int* amount_of_eqs,
                                        int* amount_of_actions,
                                        int* last_eq_pos,
                                        int* last_action_pos) {
  for (int i = 0; tree->is_token is false and i < tree->data.tree.length; i++) {
    TokenTree* subtree = &tree->data.tree.data[i];
    if (subtree->is_token and subtree->data.token.type is TOKEN_OPERATOR) {
      StrSlice op_text = subtree->data.token.data.operator_text;

      if (is_eq_operator(op_text)) {
        (*amount_of_eqs)++;
        (*last_eq_pos) = i;
      }
      if (is_action_operator(op_text)) {
        (*amount_of_actions)++;
        (*last_action_pos) = i;
      }
    }
  }
}

bool vec_char_contains(vec_char* this, char item) {
  for (int i = 0; i < this->length; i++)
    if (this->data[i] == item) return true;

  return false;
}

static str_t check_forbidden_symbols(const char* text) {
  vec_char forbidden_chars = vec_char_create();
  for (int i = 0; text[i] != '\0'; i++)
    if (not tk_is_symbol_allowed(text[i]) and
        not vec_char_contains(&forbidden_chars, text[i]))
      vec_char_push(&forbidden_chars, text[i]);

  str_t result = str_literal("");
  if (forbidden_chars.length > 0) {
    result = str_owned("Forbidden symbols: %.*s", forbidden_chars.length,
                       forbidden_chars.data);
  }
  vec_char_free(forbidden_chars);
  return result;
}

str_t calc_backend_add_expr(CalcBackend* this, const char* text) {
  debugln("calc_backend_add_expr : '%s' : Start", text);
  str_t forbidden = check_forbidden_symbols(text);
  if (strlen(forbidden.string) > 0) return forbidden;
  str_free(forbidden);
  debugln(
      "calc_backend_add_expr : '%s' : Checked for forbidden symbols. Getting "
      "context...",
      text);

  ExprContext ctx = calc_backend_get_context(this);

  debugln(
      "calc_backend_add_expr : '%s' : Got it. Dumping and then parsing "
      "TokenTree...",
      text);
  my_allocator_dump_short();
  /*debugln("calc_backend_add_expr : '%s' : SAKE. Not even gonna parse text into
  tokentree. Just gonna stop here.", text);

  CalcExpr to_add2 = {
      .type = CALC_EXPR_PLOT,
      .expression = {
        .type = EXPR_NUMBER,
        .number.value = 42.1,
      },
  };
  vec_CalcExpr_push(&this->expressions, to_add2);
  debugln("calc_backend_add_expr : '%s' : Added (2)", text);
  return str_literal("Jerkoffstone");*/

  TokenTreeResult res =
      token_tree_parse(text, ctx.is_function, ctx.context_data);
  debugln("calc_backend_add_expr : '%s' : Parsed TokenTree %s", text,
          res.is_ok ? "(successfully)" : "(failed)");
  if (not res.is_ok) return res.err.text;
  TokenTree tree = res.ok;

  debug("calc_backend_add_expr : '%s' : Got token tree", text);
  token_tree_print(&tree, DEBUG_OUT);
  debugc(". Gonna check for patters\n");
  /*
    debugln("calc_backend_add_expr : '%s' : SAKE. Just gonne ffree the token
    tree and add some trash", text); token_tree_free(tree);
    debugln("calc_backend_add_expr : '%s' : Freeed", text);

    CalcExpr to_add = {
        .type = CALC_EXPR_PLOT,
        .expression = {
          .type = EXPR_NUMBER,
          .number.value = 42.1,
        },
    };
    vec_CalcExpr_push(&this->expressions, to_add);
    debugln("calc_backend_add_expr : '%s' : Added", text);
    return str_literal("Ok (Cummington)");*/

  int amount_of_eqs = 0, amount_of_actions = 0;
  int last_eq_pos = INT_MAX, last_action_pos = INT_MAX;
  find_equalities_and_actions(&tree, &amount_of_eqs, &amount_of_actions,
                              &last_eq_pos, &last_action_pos);

  if (amount_of_eqs is 1 and last_eq_pos >= 1 and
      last_action_pos > last_eq_pos) {
    // Possibilities: variable, function
    debugln("calc_backend_add_expr : '%s' : Var or function, possibly", text);
    TokenTree* first_subtree = &tree.data.tree.data[0];
    bool is_equality = last_eq_pos is 1 and tree.data.tree.length is 3;
    bool is_variable =
        is_equality and is_unknown_ident_tree(this, first_subtree);
    bool is_function =
        is_equality and not first_subtree->is_token and
        first_subtree->data.tree.length is 2 and
        is_unknown_ident_tree(this, &first_subtree->data.tree.data[0]) and
        only_idents(&first_subtree->data.tree.data[1]);

    if (is_variable) {
      debugln("calc_backend_add_expr : '%s' : Variable indeed", text);
      return collect_variable(this, tree, last_eq_pos);
    } else if (is_function) {
      debugln("calc_backend_add_expr : '%s' : Function indeed", text);
      return collect_function(this, tree, last_eq_pos);
    } else {
      debugln("calc_backend_add_expr : '%s' : Plot indeed", text);
      return collect_plot_expr(this, tree);
    }
  } else if ((amount_of_actions is 1 and last_action_pos > 0)) {
    // Action
    debugln("calc_backend_add_expr : '%s' : Action", text);
    token_tree_free(tree);
    return str_literal("Ok (action)");
  } else {
    debugln("calc_backend_add_expr : '%s' : Plot (2) indeed", text);
    str_t result = collect_plot_expr(this, tree);
    debugln("calc_backend_add_expr : '%s' : Collected plot: '%s'", text,
            result.string);
    return result;
  }
}

bool calc_backend_is_const_expr(const CalcBackend* this, const Expr* expr) {
  if (expr->type is EXPR_VARIABLE) {
    bool isconst =
        calc_backend_is_variable_constexpr(this, expr->variable.name.string);
    return isconst;
  } else if (expr->type is EXPR_NUMBER) {
    return true;

  } else if (expr->type is EXPR_VECTOR) {
    bool result = true;
    for (int i = 0; i < expr->vector.arguments.length; i++)
      result = result and calc_backend_is_const_expr(
                              this, &expr->vector.arguments.data[i]);
    return result;

  } else if (expr->type is EXPR_BINARY_OP) {
    return calc_backend_is_const_expr(this, expr->binary_operator.lhs) and
           calc_backend_is_const_expr(this, expr->binary_operator.rhs);
  } else if (expr->type is EXPR_FUNCTION) {
    // and function is constexpr
    bool fn_const =
        calc_backend_is_function_constexpr(this, expr->function.name.string);
    bool arg_const = calc_backend_is_const_expr(this, expr->function.argument);

    return fn_const and arg_const;
  } else {
    panic("Unknown expr type");
  }
}

bool calc_backend_is_variable_constexpr(const CalcBackend* this,
                                        const char* name) {
  const CalcValue* value = calc_backend_get_value((CalcBackend*)this, name);
  if (value) return true;

  const CalcExpr* var = calc_backend_get_variable((CalcBackend*)this, name);

  // if (strcmp(name, "x") is 0 or strcmp(name, "y") is 0) return false;
  return var and calc_backend_is_const_expr(this, &var->expression);
}

bool calc_backend_is_function_constexpr(const CalcBackend* this,
                                        const char* name) {
  // debug("Check for native functions (%s)... ", name);
  const char* const native_functions[] = NATIVE_FUNCTION_NAMES;
  for (int i = 0; i < (int)LEN(native_functions); i++)
    if (strcmp(native_functions[i], name) is 0) return true;

  // debugc("Not native\n");

  // debug("Check if non existent... ");
  const CalcExpr* expr = calc_backend_get_function((CalcBackend*)this, name);
  if (not expr) return false;
  // debugc("Exists\n");

  assert_m(expr->type is CALC_EXPR_FUNCTION);
  CalcBackend local_ctx = {
      .parent = (CalcBackend*)this,
      .values = vec_CalcValue_create(),
  };

  // We add these values with no actual values just to mark
  // those as constants
  for (int i = 0; i < expr->function.args.length; i++) {
    str_t* item = &expr->function.args.data[i];
    // debugln("Adding empty value '%s'", item->string);
    vec_CalcValue_push(
        &local_ctx.values,
        (CalcValue){.name = {.is_owned = false, .string = item->string},
                    .value = {.type = EXPR_VALUE_NONE}});
  }

  // debug("Checking for constness of ");
  // expr_print(&expr->expression, DEBUG_OUT);
  // debugc("\n");
  bool c = calc_backend_is_const_expr(&local_ctx, &expr->expression);
  // debugln("Is expr itself const: %d", c);
  calc_backend_free(local_ctx);
  return c;
}
