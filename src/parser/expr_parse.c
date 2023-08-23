
#include "../util/allocator.h"
#include "expr.h"

// -- Parsing

// =====
// =
// = expr_parse_string
// =
// =====
ExprResult expr_parse_string(const char* text, ExprContext ctx) {
  assert_m(text);
  assert_m(ctx.vtable and ctx.vtable->is_function and ctx.vtable->is_variable);

  TtContext token_tree_ctx = {
      .data = ctx.data,
      .is_function = ctx.vtable->is_function,
  };
  TokenTreeResult res = token_tree_parse(text, token_tree_ctx);

  if (not res.is_ok)
    return (ExprResult){
        .is_ok = false,
        .err_text = res.err.text,
        .err_pos = res.err.text_pos,
    };

  return expr_parse_token_tree(res.ok, ctx);
}

// =====
// =
// = expr_parse_token_tree
// =
// =====
ExprResult expr_parse_token_tree(TokenTree tree, ExprContext ctx) {
  vec_TokenTree tokens_vec;

  char bracket = '<';
  if (tree.is_token) {
    tokens_vec = vec_TokenTree_create();
    vec_TokenTree_push(&tokens_vec, tree);
  } else {
    tokens_vec = tree.tree.vec;
    bracket = tree.tree.bracket;
    forget(tokens);
  }

  ExprResult res = expr_parse_tokens(tokens_vec, bracket, ctx);
  return res;
}

// =====
// =
// = expr_parse_tokens
// =
// =====

#define ExprErr(text) \
  (ExprResult) { .is_ok = false, .err_text = (text), .err_pos = null }

#define OkExprResult                                                  \
  (ExprResult) {                                                      \
    .is_ok = true, .ok = {.type = EXPR_NUMBER, .number.value = 42.0 } \
  }

static ExprResult parser_parse_item(TokenTree item, ExprContext ctx,
                                    vec_vec_Expr* result);
static ExprResult parser_map_vecvec_to_vec(vec_vec_Expr total_exprs,
                                           vec_Expr* collect_into);
static ExprResult map_vec_to_expr(vec_Expr expressions, char bracket);

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

    if (result.is_ok) result = map_vec_to_expr(values, bracket);
  } else {
    vec_vec_Expr_free(total_exprs);
  }

  return result;
}

// EXPR_PARSE_TOKENS HELPERS
static ExprResult check_for_errors(Expr* this);
static ExprResult parser_map_vecvec_to_vec(vec_vec_Expr total_exprs,
                                           vec_Expr* collect_into) {
  ExprResult result = {.is_ok = true};

  // debugln("Parsing vecvec to vec start");
  for (int pos = 0; pos < total_exprs.length and result.is_ok; pos++) {
    vec_Expr* exprs = &total_exprs.data[pos];

    if (exprs->length is 0) {
      // Empty
      result =
          ExprErr(str_owned("Empty expressions are not allowed (1) (there "
                            "are %d exprs in total)",
                            total_exprs.length));

    } else if (exprs->length > 1) {
      // Multiple
      debugln("Multiple unrelated expressions right next to each other:");
      for (int i = 0; i < exprs->length; i++)
        debugln("\t\tExpression %d: '%$expr'", i + 1, exprs->data[i]);

      result = ExprErr(str_literal(
          "Multiple unrelated expressions right next to each other"));

    } else {
      // Just right - single
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

static ExprResult map_vec_to_expr(vec_Expr values, char bracket) {
  ExprResult result = {.is_ok = true};
  if (values.length is 0) {
    result = ExprErr(str_literal("Empty expressions are not allowed (2)"));
    vec_Expr_free(values);

  } else if (values.length is 1) {
    result.ok = vec_Expr_popget(&values);
    vec_Expr_free(values);

  } else {
    result.ok = (Expr){.type = EXPR_VECTOR, .vector.arguments = values};
    /*    debugln(
            "WARNING: Non [] vector of length %d. Bracket is '%c'. Expr is "
            "'%$expr'",
            values.length, bracket, result.ok);*/
  }
  return result;
}

// EXPR_PARSE_TOKENS HELPERS HELPERS
static ExprResult check_for_errors(Expr* this) {
  assert_m(this);

  ExprResult result = OkExprResult;
  if (this->type is EXPR_NUMBER or this->type is EXPR_VARIABLE) {
    // These types just hold the data, no room for errors
    result = OkExprResult;

  } else if (this->type is EXPR_VECTOR) {
    // Vector - recursively
    for (int i = 0; i < this->vector.arguments.length and result.is_ok; i++)
      result = check_for_errors(&this->vector.arguments.data[i]);

  } else if (this->type is EXPR_FUNCTION) {
    // Function - recursively
    if (this->function.argument is null)
      result = ExprErr(str_literal("Function with no arguments"));
    else
      result = check_for_errors(this->function.argument);

  } else if (this->type is EXPR_BINARY_OP) {
    // Recursively
    const char* name = this->binary_operator.name.string;

    if (this->binary_operator.lhs is null) {
      result = ExprErr(str_owned("Incomplete operator '%s' to the left", name));
    } else if (this->binary_operator.rhs is null) {
      result =
          ExprErr(str_owned("Incomplete operator '%s' to the right", name));
    } else {
      result = check_for_errors(this->binary_operator.lhs);
      if (result.is_ok) {
        result = check_for_errors(this->binary_operator.rhs);
        if (result.is_ok) result = OkExprResult;
      }
    }
  } else
    panic("Invalid expr type");
  return result;
}

// PARSER_PARSE_ITEM

static bool has_vacant_place(const Expr* expr);
static bool has_vacant_fn_arg_place(const Expr* expr);

static bool try_push_depth(Expr* into, Expr value);
static void expr_push_to_left(vec_Expr* current_pos, Expr value);
static ExprResult expr_push_operator(vec_Expr* current_pos, Expr value,
                                     const char* err_pos);

static bool parser_is_function(TokenTree* item, ExprContext ctx);
static bool parser_is_indexing(TokenTree* item, Expr* prev);
static bool parser_is_implicit_multiplication(TokenTree* item, Expr* prev);
static bool parser_is_implicit_subtraction(TokenTree* item, Expr* prev);
static bool parser_is_unary_operator(TokenTree* item, Expr* prev);
static bool parser_is_operator(TokenTree* item, Expr* prev);
static bool parser_is_comma(TokenTree* item, Expr* prev);

static ExprResult parser_collect_function(TokenTree item, ExprContext ctx,
                                          vec_Expr* current_pos);
static ExprResult parser_collect_indexing(TokenTree item, ExprContext ctx,
                                          vec_Expr* current_pos);
static ExprResult parser_collect_implicit_mul(TokenTree item, ExprContext ctx,
                                              vec_Expr* current_pos);
static ExprResult parser_collect_implicit_sub(TokenTree item, ExprContext ctx,
                                              vec_Expr* current_pos);
static ExprResult parser_collect_unary_operator(TokenTree item, ExprContext ctx,
                                                vec_Expr* current_pos);
static ExprResult parser_collect_operator(TokenTree item, ExprContext ctx,
                                          vec_Expr* current_pos);
static ExprResult parser_collect_comma(vec_vec_Expr* result);

static ExprResult expr_parse_push_token_tree(vec_Expr* current_pos,
                                             ExprContext ctx, TokenTree item);

/*

  bool res[] = {
      parser_is_function(&item, ctx),
      parser_is_indexing(&item, prev),
      parser_is_implicit_subtraction(&item, prev),
      parser_is_implicit_multiplication(&item, prev),
      parser_is_unary_operator(&item, prev),
      parser_is_operator(&item, prev),
      parser_is_comma(&item, prev),
  };
  int old_len = current_pos->length;

  debug("TokenTree (cpos %p changed %d -> %d): '%$token_tree' characteristic:
  [", current_pos, old_len, current_pos->length, item); for (int i = 0; i <
  (int)LEN(res); i++) debugc("%c", res[i] ? 'T' : 'F'); debugc("]\n");
  debugln("Is unary: %b | Prev: %p", res[4], prev);
*/
static ExprResult parser_parse_item(TokenTree item, ExprContext ctx,
                                    vec_vec_Expr* result) {
  vec_Expr* current_pos = &result->data[result->length - 1];

  Expr* prev = current_pos->length >= 1
                   ? &current_pos->data[current_pos->length - 1]
                   : null;

  if (has_vacant_fn_arg_place(prev) and not item.is_token) {
    item.tree.bracket = '[';
  }

  ExprResult current_result;
  if (parser_is_function(&item, ctx))
    current_result = parser_collect_function(item, ctx, current_pos);

  else if (parser_is_indexing(&item, prev))
    current_result = parser_collect_indexing(item, ctx, current_pos);

  else if (parser_is_implicit_subtraction(&item, prev))
    current_result = parser_collect_implicit_sub(item, ctx, current_pos);

  else if (parser_is_implicit_multiplication(&item, prev))
    current_result = parser_collect_implicit_mul(item, ctx, current_pos);

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

// PARSER_PARSE_ITEM HELPERS
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

static ExprResult expr_parse_single_ident(Token ident, ExprContext ctx);
static ExprResult expr_parse_single_token(Token token, ExprContext ctx);

static ExprResult expr_parse_push_token_tree(vec_Expr* current_pos,
                                             ExprContext ctx, TokenTree item) {
  ExprResult result = OkExprResult;
  assert_m(current_pos);

  if (item.is_token) {
    // Parse single token and push result
    result = expr_parse_single_token(item.token, ctx);
    if (result.is_ok) {
      assert_m(result.ok.type is_not EXPR_BINARY_OP);
      expr_push_to_left(current_pos, result.ok);
      result = OkExprResult;
    }
  } else {
    // Recursively parse subtree and use it.
    // Do not free, we passed ownership to the function
    ExprResult parsed = expr_parse_token_tree(item, ctx);
    if (not parsed.is_ok) {
      result = parsed;
    } else {
      expr_push_to_left(current_pos, parsed.ok);
    }
  }
  return result;
}

static ExprResult expr_parse_single_ident(Token ident, ExprContext ctx) {
  bool is_function = ctx.vtable->is_function(ctx.data, ident.data.ident_text);
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
        "Operator (%$slice) requires arguments and cannot be used on its own",
        token.data.operator_text);
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

// PUSHES

static bool try_push_depth(Expr* into, Expr value) {
  if (into is null) return false;

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
  if (not try_push_depth(prev, value)) vec_Expr_push(current_pos, value);
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

// PARSER_PARSE_ITEM BRANCHES

//
//
//
//
// ===== FUNCTION ()
static bool parser_is_function(TokenTree* item, ExprContext ctx) {
  return token_tree_ttype(item) is TOKEN_IDENT and
         ctx.vtable->is_function(ctx.data, item->token.data.ident_text);
}

static ExprResult parser_collect_function(TokenTree item, ExprContext ctx,
                                          vec_Expr* current_pos) {
  unused(ctx);
  // It is a function and we need to combine it with the very next token
  // Like 'x sin x' or 'sin'
  Expr expr = (Expr){.type = EXPR_FUNCTION,
                     .function = {
                         .name = str_slice_to_owned(item.token.data.ident_text),
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
  return prev and not has_vacant_place(prev) and  // Prev expression is finished
         not item->is_token and
         item->tree.bracket is '[';  // AND we have smth in [brackets]
}

static ExprResult parser_collect_indexing(TokenTree item, ExprContext ctx,
                                          vec_Expr* current_pos) {
  // It is an indexing operator 'a[b]'
  assert_m(not item.is_token);
  item.tree.bracket = '{';
  ExprResult inner_res = expr_parse_token_tree(item, ctx);
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
  bool is_ident = token_tree_ttype(item) is TOKEN_IDENT;
  bool is_imp_multipliable =
      (not item->is_token or is_ident) or
      (prev and
       not(prev->type is EXPR_VARIABLE or prev->type is EXPR_FUNCTION) and
       token_tree_ttype(item) is_not TOKEN_OPERATOR and
       token_tree_ttype(item) is_not TOKEN_COMMA and
       token_tree_ttype(item) is_not TOKEN_BRACKET);

  return prev and not has_vacant_place(prev) and is_imp_multipliable;
}

static ExprResult parser_collect_implicit_mul(TokenTree item, ExprContext ctx,
                                              vec_Expr* current_pos) {
  // Implicit multiplication, like in '(x - 1)(x + 3)' or '2 x'
  // Multiplication by the next token tree
  ExprResult rhs = expr_parse_token_tree(item, ctx);
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
  return prev and token_tree_ttype(item) is TOKEN_NUMBER and
         item->token.data.number_number < 0;
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
                  .number.value = -item.token.data.number_number,
              }),
              .name = str_owned("-"),
          },
  };

  expr_push_operator(current_pos, operator, item.token.start_pos);

  // expr_push_to_left(current_pos, );
  return (ExprResult){.is_ok = true};
}

//
//
//
//
// ===== UNARY OPERATOR '- 2'
static bool parser_is_unary_operator(TokenTree* item, Expr* prev) {
  return (not prev) and token_tree_ttype(item) is TOKEN_OPERATOR and
         (str_slice_eq_ccp(item->token.data.operator_text, "+") or
          str_slice_eq_ccp(item->token.data.operator_text, "-"));
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
              .name = str_slice_to_owned(item.token.data.operator_text),
          },
  };
  expr_push_operator(current_pos, operator, item.token.start_pos);

  return (ExprResult){.is_ok = true};
}

//
//
//
//
// ===== OPERATOR a * b
static bool parser_is_operator(TokenTree* item, Expr* prev) {
  unused(prev);
  return token_tree_ttype(item) is TOKEN_OPERATOR;
}

static ExprResult parser_collect_operator(TokenTree item, ExprContext ctx,
                                          vec_Expr* current_pos) {
  unused(ctx);
  // Push operator
  if (current_pos->length is 0) {
    return (ExprResult){
        .is_ok = false,
        .err_text = str_literal("Incomplete operator"),
        .err_pos = item.token.start_pos,
    };
  }

  Expr operator= {
      .type = EXPR_BINARY_OP,
      .binary_operator =
          {
              .lhs = null,
              .rhs = null,  // This pointer will be filled later
              .name = str_slice_to_owned(item.token.data.operator_text),
          },
  };

  expr_push_operator(current_pos, operator, item.token.start_pos);

  return (ExprResult){.is_ok = true};
}

//
//
//
//
// ===== COMMA
static bool parser_is_comma(TokenTree* item, Expr* prev) {
  unused(prev);
  return token_tree_ttype(item) is TOKEN_COMMA;
}

static ExprResult parser_collect_comma(vec_vec_Expr* result) {
  // Go to next element (current_pos)
  vec_vec_Expr_push(result, vec_Expr_create());
  return (ExprResult){.is_ok = true};
}