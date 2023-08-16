#include "parser.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../util/allocator.h"
#include "../util/prettify_c.h"
#include "operators_fns.h"

#define VECTOR_C Expr
#define VECTOR_ITEM_DESTRUCTOR expr_free
#include "../util/vector.h"

#define VECTOR_C vec_Expr
#define VECTOR_ITEM_DESTRUCTOR vec_Expr_free
#include "../util/vector.h"

#define VECTOR_C ExprValue
#define VECTOR_ITEM_DESTRUCTOR expr_value_free
#include "../util/vector.h"

#define OkNum(num)                                                  \
  (ExprValueResult) {                                               \
    .is_ok = true, .ok.type = EXPR_VALUE_NUMBER, .ok.number = (num) \
  }

static ExprValueResult expr_calculate_function(const Expr* this,
                                               ExprContext ctx) {
  assert_m(this);
  assert_m(this->type is EXPR_FUNCTION);
  assert_m(ctx.call_function);
  // FUNCTION
  StrSlice function_name = str_slice_from_str_t(&this->function.name);
  ExprValueResult res = expr_calculate_val(this->function.argument, ctx);
  if (not res.is_ok) return res;

  vec_ExprValue args;

  if (res.ok.type is EXPR_VALUE_NUMBER) {
    args = vec_ExprValue_create();
    vec_ExprValue_push(&args, res.ok);

  } else if (res.ok.type is EXPR_VALUE_VEC) {
    args = res.ok.vec;

  } else if (res.ok.type is EXPR_VALUE_NONE) {
    args = vec_ExprValue_create();

  } else {
    panic("Unknown ExprValue type");
  }

  ExprValueResult call_res =
      (*ctx.call_function)(ctx.context_data, function_name, &args);
  vec_ExprValue_free(args);
  return call_res;
}

static ExprValueResult expr_calculate_vector(const Expr* this,
                                             ExprContext ctx) {
  assert_m(this);
  assert_m(this->type is EXPR_VECTOR);
  // VECTOR
  const vec_Expr* args_expr = &this->vector.arguments;
  vec_ExprValue args = vec_ExprValue_create();

  ExprValueResult res = {.is_ok = true};
  for (int i = 0; i < args_expr->length and res.is_ok; i++) {
    res = expr_calculate_val(&args_expr->data[i], ctx);

    if (res.is_ok) vec_ExprValue_push(&args, res.ok);
  }

  if (res.is_ok) {
    res = (ExprValueResult){.is_ok = true,
                            .ok = (ExprValue){
                                .type = EXPR_VALUE_VEC,
                                .vec = args,
                            }};
  } else {
    vec_ExprValue_free(args);
  }

  return res;
}

static ExprValueResult expr_calculate_binary_op(const Expr* this,
                                                ExprContext ctx) {
  assert_m(this);
  assert_m(this->type is EXPR_BINARY_OP);
  // BINARY OPERATOR
  ExprValueResult res = expr_calculate_val(this->binary_operator.lhs, ctx);
  if (res.is_ok) {
    ExprValue a = res.ok;

    res = expr_calculate_val(this->binary_operator.rhs, ctx);
    if (res.is_ok) {
      debugln("Calculated both sides, now going to op");
      ExprValue b = res.ok;
      OperatorFn operator=
          expr_get_operator_fn(this->binary_operator.name.string);
      debugln("For operator '%s' got ptr: %p",
              this->binary_operator.name.string, operator);
      assert_m(operator);

      res = operator(a, b);
      debugln("Calculated value");
    } else {
      expr_value_free(a);
    }
  }

  return res;
}

ExprValueResult expr_calculate_val(const Expr* this, ExprContext ctx) {
  // debug("Calculating value of type %d (", this->type);
  // expr_print(this, DEBUG_OUT);
  // debugc(")\n");
  assert_m(this);
  assert_m(ctx.get_variable);

  switch (this->type) {
    case EXPR_NUMBER:
      return OkNum(this->number.value);

    case EXPR_VARIABLE:
      return ctx.get_variable(ctx.context_data,
                              str_slice_from_str_t(&this->variable.name));

    case EXPR_FUNCTION:
      return expr_calculate_function(this, ctx);

    case EXPR_VECTOR:
      return expr_calculate_vector(this, ctx);

    case EXPR_BINARY_OP:
      return expr_calculate_binary_op(this, ctx);

    default:
      panic("Invalid expr type");
  }
}

static ExprResult expr_parse_single_token(Token token, ExprContext ctx);

ExprResult expr_parse_string(const char* text, ExprContext ctx) {
  assert_m(text);
  assert_m(ctx.is_function);

  TokenTreeResult res =
      token_tree_parse(text, ctx.is_function, ctx.context_data);
  if (not res.is_ok)
    return (ExprResult){
        .is_ok = false,
        .err_text = res.err.text,
        .err_pos = res.err.text_pos,
    };
  TokenTree tokentree = res.ok;

  ExprResult result;
  if (tokentree.is_token) {
    result = expr_parse_single_token(tokentree.data.token, ctx);
  } else {
    vec_TokenTree tokens = tokentree.data.tree;

    result = expr_parse_tokens(tokens, tokentree.data.tree_bracket, ctx);
  }

  // No need to free, we passed ownership
  return result;
}

static Expr* expr_move_to_heap(Expr value) {
  Expr* ptr = (Expr*)MALLOC(sizeof(Expr));
  assert_alloc(ptr);
  (*ptr) = value;
  return ptr;
}

static bool try_push_depth(Expr* into, Expr value) {
  if (into is null) return false;

  // printf("Push '");
  // expr_print(&value, stdout);
  // printf("' depth: into type is %d. Expr is: '", into->type);
  // expr_print(into, stdout);
  // printf("' <-\n");
  if (into->type is EXPR_BINARY_OP) {
    if (into->binary_operator.rhs is null) {
      into->binary_operator.rhs = expr_move_to_heap(value);
      return true;
    } else {
      return try_push_depth(into->binary_operator.rhs, value);
    }
  } else if (into->type is EXPR_FUNCTION) {
    if (into->function.argument is null) {
      into->function.argument = expr_move_to_heap(value);
      return true;
    } else {
      return try_push_depth(into->function.argument, value);
    }
  } else {
    return false;
  }
}

static void expr_push_to_left(vec_Expr* current_pos, Expr value) {
  assert_m(current_pos);
  Expr* prev = current_pos->length > 0
                   ? &current_pos->data[current_pos->length - 1]
                   : null;

  // debug("+-+-+-+ Pushing element: '");
  // expr_print(&value, DEBUG_OUT);
  // debugc("'\n");
  if (not try_push_depth(prev, value)) vec_Expr_push(current_pos, value);
}

#define OkExprResult                                                  \
  (ExprResult) {                                                      \
    .is_ok = true, .ok = {.type = EXPR_NUMBER, .number.value = 42.0 } \
  }
static ExprResult expr_push_operator(vec_Expr* current_pos, Expr value,
                                     const char* err_pos) {
  assert_m(value.type is EXPR_BINARY_OP);
  assert_m(current_pos);

  if (value.binary_operator.lhs is null) {
    if (current_pos->length is 0) {
      expr_free(value);
      return (ExprResult){
          .is_ok = false,
          .err_text = str_owned("Incomplete operator '%s' (to the left)",
                                value.binary_operator.name.string),
          .err_pos = err_pos,
      };
    } else {
      value.binary_operator.lhs =
          expr_move_to_heap(vec_Expr_popget(current_pos));
    }
  }

  vec_Expr_push(current_pos, value);
  return OkExprResult;
}

static ExprResult expr_parse_push_token_tree(vec_Expr* current_pos,
                                             ExprContext ctx, TokenTree item) {
  ExprResult result = OkExprResult;
  assert_m(current_pos);

  if (item.is_token) {
    // Parse single token and push result
    result = expr_parse_single_token(item.data.token, ctx);
    if (result.is_ok) {
      assert_m(result.ok.type is_not EXPR_BINARY_OP);
      expr_push_to_left(current_pos, result.ok);
      result = OkExprResult;
    }
  } else {
    // Recursively parse subtree and use it.
    // Do not free, we passed ownership to the function
    /*debug("Starting to parse a tree with bracket '%c': '",
          item.data.tree_bracket);
    token_tree_print(item, DEBUG_OUT);
    debugc("'\n");*/

    ExprResult parsed =
        expr_parse_tokens(item.data.tree, item.data.tree_bracket, ctx);

    debugln("Done parsing!");
    if (not parsed.is_ok) {
      result = parsed;
    } else {
      expr_push_to_left(current_pos, parsed.ok);
      result = OkExprResult;
    }
  }
  return result;
}

static ExprResult check_for_errors(Expr* this) {
  assert_m(this);

  ExprResult result = OkExprResult;
  if (this->type is EXPR_NUMBER or this->type is EXPR_VARIABLE) {
    // These types just hold the data, no room for errors
    result = OkExprResult;

  } else if (this->type is EXPR_VECTOR) {
    for (int i = 0; i < this->vector.arguments.length and result.is_ok; i++) {
      result = check_for_errors(&this->vector.arguments.data[i]);
    }

  } else if (this->type is EXPR_FUNCTION) {
    if (this->function.argument is null) {
      result =
          (ExprResult){.is_ok = false,
                       .err_text = str_literal("Function with no arguments")};
    } else {
      result = check_for_errors(this->function.argument);
    }

  } else if (this->type is EXPR_BINARY_OP) {
    ExprResult check;
    if (this->binary_operator.lhs is null) {
      result = (ExprResult){
          .is_ok = false,
          .err_text = str_owned("Incomplete operator '%s' to the left",
                                this->binary_operator.name.string)};
    } else if (this->binary_operator.rhs is null) {
      result = (ExprResult){
          .is_ok = false,
          .err_text = str_owned("Incomplete operator '%s' to the right",
                                this->binary_operator.name.string)};
    } else if ((check = check_for_errors(this->binary_operator.lhs))
                   .is_ok is_not true) {
      result = check;
    } else if ((check = check_for_errors(this->binary_operator.rhs))
                   .is_ok is_not true) {
      result = check;
    }

  } else {
    panic("Invalid expr type");
  }
  return result;
}

static bool has_vacant_place(const Expr* expr) {
  if (not expr) return false;

  if (expr->type is EXPR_BINARY_OP) {
    if (expr->binary_operator.rhs is null)
      return true;
    else
      return has_vacant_place(expr->binary_operator.rhs);

  } else if (expr->type is EXPR_FUNCTION) {
    if (expr->function.argument is null)
      return true;
    else
      return has_vacant_place(expr->function.argument);
  } else {
    return false;
  }
}

static bool has_vacant_fn_arg_place(const Expr* expr) {
  if (not expr) return false;

  if (expr->type is EXPR_BINARY_OP) {
    if (expr->binary_operator.rhs is null)
      return false;
    else
      return has_vacant_fn_arg_place(expr->binary_operator.rhs);

  } else if (expr->type is EXPR_FUNCTION) {
    if (expr->function.argument is null)
      return true;
    else
      return has_vacant_fn_arg_place(expr->function.argument);
  } else {
    return false;
  }
}

ExprResult expr_parse_token_tree(TokenTree tokens, ExprContext ctx) {
  vec_TokenTree tokens_vec;

  char bracket = '<';
  if (tokens.is_token) {
    tokens_vec = vec_TokenTree_create();
    vec_TokenTree_push(&tokens_vec, tokens);
  } else {
    tokens_vec = tokens.data.tree;
    bracket = tokens.data.tree_bracket;
    forget(tokens);
  }

  return expr_parse_tokens(tokens_vec, bracket, ctx);
}

//
//
//
//
// ===== FUNCTION ()
static bool parser_is_function(TokenTree* item, ExprContext ctx) {
  return tt_token_type(item) is TOKEN_IDENT and
         ctx.is_function(ctx.context_data, item->data.token.data.ident_text);
}

static ExprResult parser_collect_function(TokenTree item, ExprContext ctx,
                                          vec_Expr* current_pos) {
  unused(ctx);
  // It is a function and we need to combine it with the very next token
  // Like 'x sin x' or 'sin'
  Expr expr =
      (Expr){.type = EXPR_FUNCTION,
             .function = {
                 .name = str_slice_to_owned(item.data.token.data.ident_text),
                 .argument = null,  // This pointer will be filled later
             }};
  expr_push_to_left(current_pos, expr);

  return (ExprResult){.is_ok = true};
}

//
//
//
//
// ===== INDEXING OPERATOR []
static bool parser_is_indexing(TokenTree* item, Expr* prev) {
  return prev and not has_vacant_place(prev) and not item->is_token and
         item->data.tree_bracket is '[';
}

static ExprResult parser_collect_indexing(TokenTree item, ExprContext ctx,
                                          vec_Expr* current_pos) {
  // It is an indexing operator 'a[b]'
  assert_m(not item.is_token);
  item.data.tree_bracket = '{';
  ExprResult inner_res =
      expr_parse_tokens(item.data.tree, item.data.tree_bracket, ctx);
  if (not inner_res.is_ok) return inner_res;

  Expr expr = {
      .type = EXPR_BINARY_OP,
      .binary_operator =
          {
              .lhs = null,
              .rhs = null,  // This pointer will be filled later
              .name = str_owned("[]"),
          },
  };

  expr_push_operator(current_pos, expr, null);
  expr_push_to_left(current_pos, inner_res.ok);

  return (ExprResult){.is_ok = true};
}

//
//
//
//
// ===== IMPLICIT MULTIPLICATION (a)(b)
static bool parser_is_implicit_multiplication(TokenTree* item, Expr* prev) {
  bool is_ident = tt_token_type(item) is TOKEN_IDENT;
  bool is_imp_multipliable =
      (not item->is_token or is_ident) or
      (prev and
       not(prev->type is EXPR_VARIABLE or prev->type is EXPR_FUNCTION) and
       tt_token_type(item) is_not TOKEN_OPERATOR and
       tt_token_type(item) is_not TOKEN_COMMA and
       tt_token_type(item) is_not TOKEN_BRACKET);

  return prev and not has_vacant_place(prev) and is_imp_multipliable;
}

static ExprResult parser_collect_implicit_mul(TokenTree item, ExprContext ctx,
                                              vec_Expr* current_pos) {
  // Implicit multiplication, like in '(x - 1)(x + 3)' or '2 x'
  // Multiplication by the next token tree
  ExprResult rhs;

  if (item.is_token)
    rhs = expr_parse_single_token(item.data.token, ctx);
  else
    rhs = expr_parse_tokens(item.data.tree, item.data.tree_bracket, ctx);

  if (not rhs.is_ok) return rhs;

  Expr expr = {
      .type = EXPR_BINARY_OP,
      .binary_operator =
          {
              .lhs = null,
              .rhs = null,
              .name = str_owned("*"),
          },
  };

  expr_push_operator(current_pos, expr, null);
  expr_push_to_left(current_pos, rhs.ok);
  return (ExprResult){.is_ok = true};
}

//
//
//
//
// ===== IMPLICIT SUBTRACTION a -2
static bool parser_is_implicit_subtraction(TokenTree* item, Expr* prev) {
  return prev and tt_token_type(item) is TOKEN_NUMBER and
         item->data.token.data.number_number < 0;
}

static ExprResult parser_collect_implicit_sub(TokenTree item, ExprContext ctx,
                                              vec_Expr* current_pos) {
  unused(ctx);
  // Implicit subtraction like 'x -2' ('x' is a token, and '-2' is a token)
  Expr operator= {
      .type = EXPR_BINARY_OP,
      .binary_operator =
          {
              .lhs = null,
              .rhs = expr_move_to_heap((Expr){
                  .type = EXPR_NUMBER,
                  .number.value = -item.data.token.data.number_number,
              }),
              .name = str_owned("-"),
          },
  };

  expr_push_operator(current_pos, operator, item.data.token.start_pos);

  // expr_push_to_left(current_pos, );
  return (ExprResult){.is_ok = true};
}

//
//
//
//
// ===== UNARY OPERATOR '- 2'
static bool parser_is_unary_operator(TokenTree* item, Expr* prev) {
  return (not prev) and tt_token_type(item) is TOKEN_OPERATOR and
         (str_slice_eq_ccp(item->data.token.data.operator_text, "+") or
          str_slice_eq_ccp(item->data.token.data.operator_text, "-"));
}

static ExprResult parser_collect_unary_operator(TokenTree item, ExprContext ctx,
                                                vec_Expr* current_pos) {
  unused(ctx);
  // It is unary sign operator (if there is something after this token)
  Expr operator= {
      .type = EXPR_BINARY_OP,
      .binary_operator =
          {
              .lhs = expr_move_to_heap((Expr){
                  .type = EXPR_NUMBER,
                  .number.value = 0.0,
              }),
              .rhs = null,  // This pointer will be filled later
              .name = str_slice_to_owned(item.data.token.data.operator_text),
          },
  };
  expr_push_operator(current_pos, operator, item.data.token.start_pos);

  return (ExprResult){.is_ok = true};
}

//
//
//
//
// ===== OPERATOR a * b
static bool parser_is_operator(TokenTree* item, Expr* prev) {
  unused(prev);
  return tt_token_type(item) is TOKEN_OPERATOR;
}

static ExprResult parser_collect_operator(TokenTree item, ExprContext ctx,
                                          vec_Expr* current_pos) {
  unused(ctx);
  // Push operator
  if (current_pos->length is 0) {
    return (ExprResult){
        .is_ok = false,
        .err_text = str_literal("Incomplete operator"),
        .err_pos = item.data.token.start_pos,
    };
  }

  Expr operator= {
      .type = EXPR_BINARY_OP,
      .binary_operator =
          {
              .lhs = null,
              .rhs = null,  // This pointer will be filled later
              .name = str_slice_to_owned(item.data.token.data.operator_text),
          },
  };

  expr_push_operator(current_pos, operator, item.data.token.start_pos);

  return (ExprResult){.is_ok = true};
}

//
//
//
//
// ===== COMMA
static bool parser_is_comma(TokenTree* item, Expr* prev) {
  unused(prev);
  return tt_token_type(item) is TOKEN_COMMA;
}

static ExprResult parser_collect_comma(vec_vec_Expr* result) {
  // Go to next element (current_pos)
  vec_vec_Expr_push(result, vec_Expr_create());
  return (ExprResult){.is_ok = true};
}

#define ExprErr(literal)                                                \
  (ExprResult) {                                                        \
    .is_ok = false, .err_text = str_literal((literal)), .err_pos = null \
  }

static ExprResult parser_parse_item(TokenTree item, ExprContext ctx,
                                    vec_vec_Expr* result) {
  vec_Expr* current_pos = &result->data[result->length - 1];

  Expr* prev = current_pos->length >= 1
                   ? &current_pos->data[current_pos->length - 1]
                   : null;

  if (has_vacant_fn_arg_place(prev) and not item.is_token) {
    item.data.tree_bracket = '[';
  }
  /*
    debug("TokenTree: "); token_tree_print(item, DEBUG_OUT); debugc("\n");
    debugln("is_function: %d", parser_is_function(&item, ctx));
    debugln("is_indexing: %d", parser_is_indexing(&item, prev));
    debugln("is_implicit_multiplication: %d",
    parser_is_implicit_multiplication(&item, prev));
    debugln("is_implicit_subtraction: %d", parser_is_implicit_subtraction(&item,
    prev)); debugln("is_unary_operator: %d", parser_is_unary_operator(&item,
    prev)); debugln("is_operator: %d", parser_is_operator(&item, prev));
    debugln("is_comma: %d", parser_is_comma(&item, prev));
  */
  ExprResult current_result;
  if (parser_is_function(&item, ctx))
    current_result = parser_collect_function(item, ctx, current_pos);

  else if (parser_is_indexing(&item, prev))
    current_result = parser_collect_indexing(item, ctx, current_pos);

  else if (parser_is_implicit_multiplication(&item, prev))
    current_result = parser_collect_implicit_mul(item, ctx, current_pos);

  else if (parser_is_implicit_subtraction(&item, prev))
    current_result = parser_collect_implicit_sub(item, ctx, current_pos);

  else if (parser_is_unary_operator(&item, prev))
    current_result = parser_collect_unary_operator(item, ctx, current_pos);

  else if (parser_is_operator(&item, prev))
    current_result = parser_collect_operator(item, ctx, current_pos);

  else if (parser_is_comma(&item, prev))
    current_result = parser_collect_comma(result);

  else
    current_result = expr_parse_push_token_tree(current_pos, ctx, item);

  return current_result;
}

static ExprResult parser_map_vecvec_to_vec(vec_vec_Expr total_exprs,
                                           vec_Expr* collect_into) {
  ExprResult result = {.is_ok = true};

  // debugln("Parsing vecvec to vec start");
  for (int pos = 0; pos < total_exprs.length and result.is_ok; pos++) {
    vec_Expr* exprs = &total_exprs.data[pos];

    if (exprs->length is 0) {
      debugln("Empty expression indeed");
      result = (ExprResult){
          .is_ok = false,
          .err_pos = null,
          .err_text = str_owned("Empty expressions are not allowed (1) (there "
                                "are %d exprs in total)",
                                total_exprs.length)};
    } else if (exprs->length > 1) {
      debugln("Multiple unrelated expressions right next to each other:");
      for (int i = 0; i < exprs->length; i++) {
        debug("\t\tExpression %d: '", i + 1);
        expr_print(&exprs->data[i], DEBUG_OUT);
        debugc("'\n");
      }
      result =
          ExprErr("Multiple unrelated expressions right next to each other");
    } else {
      ExprResult check = check_for_errors(&exprs->data[0]);
      if (not check.is_ok) {
        result = check;
        break;
      }

      vec_Expr_push(collect_into, vec_Expr_popget(exprs));
    }
  }
  vec_vec_Expr_free(total_exprs);
  // debugln("Parsing vecvec to vec end\n");

  return result;
}

//
//
//
//
// ===== GLOBAL PARSING
ExprResult expr_parse_tokens(vec_TokenTree tokens, char bracket,
                             ExprContext ctx) {
  ExprResult result = {.is_ok = true};

  // Mapping vec of TokenTrees to vec of Exprs and operators one-after-another
  vec_vec_Expr total_exprs = vec_vec_Expr_create();
  vec_vec_Expr_push(&total_exprs, vec_Expr_create());

  foreach_extract(TokenTree item, tokens, result.is_ok, {  //
    result = parser_parse_item(item, ctx, &total_exprs);   //
  });                                                      //
  vec_TokenTree_free(tokens);

  if (result.is_ok) {
    vec_Expr values = vec_Expr_create();
    result = parser_map_vecvec_to_vec(total_exprs, &values);

    if (result.is_ok) {
      if (values.length is 0) {
        result = ExprErr("Empty expressions are not allowed (2)");
        vec_Expr_free(values);
      } else if (bracket is '[') {
        result.ok = (Expr){.type = EXPR_VECTOR, .vector.arguments = values};
      } else if (values.length is 1 or true) {
        if (values.length != 1)
          debugln("WARNING: NON [] vector of length %d. Bracket is '%c'",
                  values.length, bracket);
        result.ok = vec_Expr_popget(&values);
        vec_Expr_free(values);
      }
    }
  } else {
    vec_vec_Expr_free(total_exprs);
  }

  return result;
}

#undef ExprErr

#define ExprErr(text)                                              \
  (ExprResult) {                                                   \
    .is_ok = false, .err_text = (text), .err_pos = token.start_pos \
  }

static ExprResult expr_parse_single_ident(Token ident, ExprContext ctx) {
  bool is_function = ctx.is_function(ctx.context_data, ident.data.ident_text);
  ExprResult result = {.is_ok = true};

  if (is_function) {
    result = (ExprResult){
        .is_ok = false,
        .err_pos = ident.start_pos,
        .err_text = str_literal("Incomplete function (2)"),
    };
  } else {
    result.ok.type = EXPR_VARIABLE;
    result.ok.variable.name = str_slice_to_owned(ident.data.ident_text);
  }

  return result;
}

static ExprResult expr_parse_single_token(Token token, ExprContext ctx) {
  if (token.type is TOKEN_BRACKET) {
    return ExprErr(str_literal("Bracket on its own cannot be parsed"));

  } else if (token.type is TOKEN_COMMA) {
    return ExprErr(str_literal("Comma cannot be parsed on its own"));

  } else if (token.type is TOKEN_OPERATOR) {
    str_t error_text = str_owned(
        "Operator (%.*s) requires arguments and cannot be used on its own",
        token.data.operator_text.length, token.data.operator_text.start);
    return ExprErr(error_text);

  } else if (token.type is TOKEN_NUMBER) {
    Expr number = {.type = EXPR_NUMBER,
                   .number.value = token.data.number_number};
    return (ExprResult){.is_ok = true, .ok = number};

  } else if (token.type is TOKEN_IDENT) {
    return expr_parse_single_ident(token, ctx);

  } else {
    panic("Unknown token type: %d", token.type);
  }
}

#undef ExprErr

void expr_free(Expr this) {
  if (this.type is EXPR_NUMBER) {
    // Number does not OWN any resources to free

  } else if (this.type is EXPR_VARIABLE) {
    str_free(this.variable.name);

  } else if (this.type is EXPR_FUNCTION) {
    str_free(this.function.name);
    if (this.function.argument) {
      expr_free(*this.function.argument);
      FREE(this.function.argument);
    }

  } else if (this.type is EXPR_VECTOR) {
    vec_Expr_free(this.vector.arguments);

  } else if (this.type is EXPR_BINARY_OP) {
    str_free(this.binary_operator.name);

    if (this.binary_operator.lhs) expr_free(*this.binary_operator.lhs);
    if (this.binary_operator.rhs) expr_free(*this.binary_operator.rhs);
    FREE(this.binary_operator.lhs);
    FREE(this.binary_operator.rhs);

  } else {
    panic("Invalid expr type");
  }
}

void expr_value_free(ExprValue this) {
  // debug("Freeing ExprValue of type %d (%s)...", this.type,
  // expr_value_type_text(this.type));
  if (this.type is EXPR_VALUE_NUMBER) {
  } else if (this.type is EXPR_VALUE_VEC) {
    vec_ExprValue_free(this.vec);

  } else if (this.type is EXPR_VALUE_NONE) {
  } else {
    panic("Unknown ExprValue type");
  }
  // debugc("Done!\n");
}

ExprValue expr_value_clone(const ExprValue* source) {
  ExprValue res;
  res.type = source->type;

  if (res.type is EXPR_VALUE_NUMBER) {
    res.number = source->number;
  } else if (res.type is EXPR_VALUE_VEC) {
    res.vec = vec_ExprValue_create_copy(source->vec.data, source->vec.length);
  } else if (res.type is EXPR_VALUE_NONE) {
    // do nothing
  } else {
    panic("Unknown ExprValue type");
  }

  return res;
}

VecExprResult ExprResult_into_VecExprResult(ExprResult from) {
  if (from.is_ok) {
    vec_Expr vec = vec_Expr_create();
    vec_Expr_push(&vec, from.ok);
    return (VecExprResult){.is_ok = from.is_ok, .ok = vec};
  } else {
    return (VecExprResult){
        .is_ok = from.is_ok,
        .err_pos = from.err_pos,
        .err_text = from.err_text,
    };
  }
}

static void expr_print_binary_op(const Expr* this, OutStream out) {
  assert_m(this->type is EXPR_BINARY_OP);

  outstream_putc('(', out);
  expr_print(this->binary_operator.lhs, out);

  if (strcmp(this->binary_operator.name.string, "[]") == 0) {
    x_sprintf(out, "[");
    expr_print(this->binary_operator.rhs, out);
    x_sprintf(out, "]");
  } else {
    x_sprintf(out, " %s ", this->binary_operator.name.string);
    expr_print(this->binary_operator.rhs, out);
  }

  outstream_putc(')', out);
}

void expr_print(const Expr* this, OutStream out) {
  if (this is null) {
    x_sprintf(out, "<nullptr>");
    return;
  }
  if (this->type is EXPR_NUMBER) {
    x_sprintf(out, "%.1lf", this->number.value);

  } else if (this->type is EXPR_VARIABLE) {
    x_sprintf(out, "%s", this->variable.name.string);
  } else if (this->type is EXPR_FUNCTION) {
    x_sprintf(out, "<%s> ", this->function.name.string);
    expr_print(this->function.argument, out);

  } else if (this->type is EXPR_VECTOR) {
    outstream_putc('[', out);

    for (int i = 0; i < this->vector.arguments.length; i++) {
      if (i > 0) x_sprintf(out, ", ");

      expr_print(&this->vector.arguments.data[i], out);
    }
    outstream_putc(']', out);

  } else if (this->type is EXPR_BINARY_OP) {
    expr_print_binary_op(this, out);

  } else {
    panic("Invalid expr type");
  }
}

#define BUFFER_SIZE (16 * 1024)
static char* str_print(char* old_text, const char* format, ...) {
  char buffer[BUFFER_SIZE] = {'\0'};

  va_list list;
  va_start(list, format);
  vsprintf(buffer, format, list);
  buffer[BUFFER_SIZE - 1] = '\0';
  va_end(list);

  int old_len = old_text ? strlen(old_text) : 0;
  int add_len = strlen(buffer);

  old_text = (char*)REALLOC(old_text, (old_len + add_len + 1) * sizeof(char));
  strcpy(old_text + old_len, buffer);
  return old_text;
}

static char* expr_str_print_binary_op(const Expr* this, char* out) {
  assert_m(this->type is EXPR_BINARY_OP);

  out = str_print(out, "(");
  out = expr_str_print(this->binary_operator.lhs, out);

  if (strcmp(this->binary_operator.name.string, "[]") == 0) {
    out = str_print(out, "[");
    out = expr_str_print(this->binary_operator.rhs, out);
    out = str_print(out, "]");
  } else {
    out = str_print(out, " %s ", this->binary_operator.name.string);
    out = expr_str_print(this->binary_operator.rhs, out);
  }

  out = str_print(out, ")");
  return out;
}

char* expr_str_print(const Expr* this, char* out) {
  if (this is null) {
    out = str_print(out, "<nullptr>");
    return out;
  }

  if (this->type is EXPR_NUMBER) {
    out = str_print(out, "%.1lf", this->number.value);

  } else if (this->type is EXPR_VARIABLE) {
    out = str_print(out, "%s", this->variable.name.string);

  } else if (this->type is EXPR_FUNCTION) {
    out = str_print(out, "<%s> ", this->function.name.string);
    out = expr_str_print(this->function.argument, out);

  } else if (this->type is EXPR_VECTOR) {
    out = str_print(out, "[");

    for (int i = 0; i < this->vector.arguments.length; i++) {
      if (i > 0) out = str_print(out, ", ");

      out = expr_str_print(&this->vector.arguments.data[i], out);
    }
    out = str_print(out, "]");

  } else if (this->type is EXPR_BINARY_OP) {
    out = expr_str_print_binary_op(this, out);

  } else {
    panic("Invalid expr type");
  }

  return out;
}

void expr_value_print(const ExprValue* this, OutStream stream) {
  if (this->type is EXPR_VALUE_NUMBER) {
    x_sprintf(stream, "%.2lf", this->number);

  } else if (this->type is EXPR_VALUE_VEC) {
    outstream_putc('[', stream);
    for (int i = 0; i < this->vec.length; i++) {
      if (i > 0) outstream_puts(", ", stream);

      expr_value_print(&this->vec.data[i], stream);
    }
    outstream_putc(']', stream);

  } else if (this->type is EXPR_VALUE_NONE) {
    outstream_puts("()", stream);

  } else {
    panic("Unknown ExprValue type");
  }
}

char* expr_value_str_print(const ExprValue* this, char* old_string) {
  if (this->type is EXPR_VALUE_NUMBER) {
    old_string = str_print(old_string, "%lf", this->number);

  } else if (this->type is EXPR_VALUE_VEC) {
    old_string = str_print(old_string, "[");
    for (int i = 0; i < this->vec.length; i++) {
      if (i > 0) old_string = str_print(old_string, ", ");

      old_string = expr_value_str_print(&this->vec.data[i], old_string);
    }
    old_string = str_print(old_string, "]");

  } else if (this->type is EXPR_VALUE_NONE) {
    old_string = str_print(old_string, "()");

  } else {
    panic("Unknown ExprValue type");
  }
  return old_string;
}

static bool vstrt_contains(const vec_str_t* vec, str_t* value) {
  for (int i = 0; i < vec->length; i++)
    if (strcmp(vec->data[i].string, value->string) == 0) return true;

  return false;
}

static void vstrt_push_unique(vec_str_t* into, vec_str_t from) {
  for (int i = 0; i < from.length; i++) {
    str_t var = vec_str_t_popget(&from);
    if (not vstrt_contains(into, &var))
      vec_str_t_push(into, var);
    else
      str_free(var);
  }

  vec_str_t_free(from);
}

vec_str_t expr_get_used_variables(const Expr* this) {
  vec_str_t result = vec_str_t_create();

  if (this->type is EXPR_FUNCTION or this->type is EXPR_NUMBER) {
    // nothing
  } else if (this->type is EXPR_VARIABLE) {
    vec_str_t_push(&result, str_clone(&this->variable.name));

  } else if (this->type is EXPR_VECTOR) {
    for (int el = 0; el < this->vector.arguments.length; el++) {
      vec_str_t variables =
          expr_get_used_variables(&this->vector.arguments.data[el]);
      vstrt_push_unique(&result, variables);
    }

  } else if (this->type is EXPR_BINARY_OP) {
    vstrt_push_unique(&result,
                      expr_get_used_variables(this->binary_operator.lhs));
    vstrt_push_unique(&result,
                      expr_get_used_variables(this->binary_operator.rhs));

  } else {
    panic("Unknown expr type");
  }

  return result;
}

vec_str_t expr_get_used_functions(const Expr* this) {
  vec_str_t result = vec_str_t_create();

  if (this->type is EXPR_VARIABLE or this->type is EXPR_NUMBER) {
    // nothing
  } else if (this->type is EXPR_FUNCTION) {
    vec_str_t_push(&result, str_clone(&this->function.name));

  } else if (this->type is EXPR_VECTOR) {
    for (int el = 0; el < this->vector.arguments.length; el++) {
      vec_str_t variables =
          expr_get_used_functions(&this->vector.arguments.data[el]);
      vstrt_push_unique(&result, variables);
    }

  } else if (this->type is EXPR_BINARY_OP) {
    vstrt_push_unique(&result,
                      expr_get_used_functions(this->binary_operator.lhs));
    vstrt_push_unique(&result,
                      expr_get_used_functions(this->binary_operator.rhs));

  } else {
    panic("Unknown expr type");
  }

  return result;
}

void expr_iter_variables(const Expr* this,
                         void (*callback)(void* cb_data, const str_t* var_name),
                         void* cb_data) {
  if (this->type is EXPR_VARIABLE) {
    callback(cb_data, &this->variable.name);
  } else if (this->type is EXPR_NUMBER) {
  } else if (this->type is EXPR_VECTOR) {
    for (int i = 0; i < this->vector.arguments.length; i++)
      expr_iter_variables(&this->vector.arguments.data[i], callback, cb_data);
  } else if (this->type is EXPR_BINARY_OP) {
    expr_iter_variables(this->binary_operator.lhs, callback, cb_data);
    expr_iter_variables(this->binary_operator.rhs, callback, cb_data);
  } else if (this->type is EXPR_FUNCTION) {
    expr_iter_variables(this->function.argument, callback, cb_data);
  } else {
    panic("Unknown expr type");
  }
}

const char* expr_value_type_text(int type) {
  switch (type) {
    case EXPR_VALUE_NONE:
      return "none";
    case EXPR_VALUE_NUMBER:
      return "number";
    case EXPR_VALUE_VEC:
      return "vec";
    default:
      return "unknown-type";
  }
}
