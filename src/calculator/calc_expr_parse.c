#include <stdint.h>

#include "../util/allocator.h"
#include "../util/common_vecs.h"
#include "../util/prettify_c.h"
#include "calc_backend.h"
#include "calc_expr.h"
#include "func_const_ctx.h"

static bool is_action(TokenTree* tree);
static bool is_variable(ExprContext ctx, TokenTree* tree);
static bool is_function(ExprContext ctx, TokenTree* tree);

static CalcExprResult parse_action(ExprContext ctx, TokenTree tree);
static CalcExprResult parse_variable(ExprContext ctx, TokenTree tree);
static CalcExprResult parse_function(ExprContext ctx, TokenTree tree);
static CalcExprResult parse_plot(ExprContext ctx, TokenTree tree);

#define ASCII_MAX 127
bool vec_char_contains(vec_char* this, char item);
static str_t check_forbidden_symbols(const char* text);

CalcExprResult calc_expr_parse(ExprContext ctx, const char* text) {
  str_t forbidden = check_forbidden_symbols(text);
  if (strlen(forbidden.string) > 0) return CalcExprErr(null, forbidden);
  str_free(forbidden);

  TtContext tt_ctx = {
      .data = ctx.data,
      .is_function = ctx.vtable->is_function,
  };
  TokenTreeResult res = token_tree_parse(text, tt_ctx);
  // debugln("Got token tree: %$token_tree", res.ok);

  if (not res.is_ok) return CalcExprErr(res.err.text_pos, res.err.text);
  return calc_expr_parse_tt(ctx, res.ok);
}

CalcExprResult calc_expr_parse_tt(ExprContext ctx, TokenTree tree) {
  CalcExprResult result;

  if (is_action(&tree))
    result = parse_action(ctx, tree);
  else if (is_variable(ctx, &tree))
    result = parse_variable(ctx, tree);
  else if (is_function(ctx, &tree))
    result = parse_function(ctx, tree);
  else
    result = parse_plot(ctx, tree);

  return result;
}

static str_t check_forbidden_symbols(const char* text) {
  vec_char forbidden_chars = vec_char_create();
  for (int i = 0; text[i] != '\0'; i++)
    if (not tk_is_symbol_allowed(text[i]) and
        not vec_char_contains(&forbidden_chars, text[i]))
      vec_char_push(&forbidden_chars,
                    (uint8_t)text[i] > (uint8_t)ASCII_MAX ? '?' : text[i]);

  str_t result = str_literal("");
  if (forbidden_chars.length > 0) {
    result = str_owned("Forbidden symbols: %.*s", forbidden_chars.length,
                       forbidden_chars.data);
  }
  vec_char_free(forbidden_chars);
  return result;
}

bool vec_char_contains(vec_char* this, char item) {
  for (int i = 0; i < this->length; i++)
    if (this->data[i] == item) return true;

  return false;
}

// =====

static bool is_action_operator(StrSlice op_text);

static bool is_action(TokenTree* tree) {
  if (tree->is_token) {
    return tree->token.type is TOKEN_OPERATOR and
           is_action_operator(tree->token.data.operator_text);
  } else {
    bool result = false;

    for (int i = 0; i < tree->tree.vec.length and not result; i++)
      result = is_action(&tree->tree.vec.data[i]);

    return result;
  }
}

static CalcExprResult parse_action(ExprContext ctx, TokenTree tree) {
  unused(ctx);
  unused(tree);
  return CalcExprErr(null, str_literal("Actions are not supported yet! TODO"));
}

static bool is_action_operator(StrSlice op_text) {
  return str_slice_eq_ccp(op_text, ":=") or str_slice_eq_ccp(op_text, "+=") or
         str_slice_eq_ccp(op_text, "-=") or str_slice_eq_ccp(op_text, "*=") or
         str_slice_eq_ccp(op_text, "/=") or str_slice_eq_ccp(op_text, "%=") or
         str_slice_eq_ccp(op_text, "^=");
}

// =====

// VARIABLE
static bool is_unknown_ident(ExprContext ctx, StrSlice text);
static bool is_eq_operator(StrSlice op_text);
static bool is_eq_tt(TokenTree* tree);

static bool is_variable(ExprContext ctx, TokenTree* tree) {
  // 1. Unknown ident
  // 2. Equality sign
  // ... rest garbage
  if (tree->is_token or tree->tree.vec.length != 3) return false;

  Token* var_name = token_tree_get_only_token(&tree->tree.vec.data[0]);
  if (not var_name or var_name->type is_not TOKEN_IDENT) return false;

  return is_unknown_ident(ctx, var_name->data.ident_text) and
         is_eq_tt(&tree->tree.vec.data[1]);
}

static CalcExprResult parse_variable(ExprContext ctx, TokenTree tree) {
  assert_m(not tree.is_token and tree.tree.vec.length >= 3);

  // Name
  str_t var_name;
  {
    TokenTree first = vec_TokenTree_extract_order(&tree.tree.vec, 0);
    Token* var_name_token = token_tree_get_only_token(&first);
    assert_m(var_name_token and var_name_token->type is TOKEN_IDENT);
    var_name = str_slice_to_owned(var_name_token->data.ident_text);
    token_tree_free(first);
  }

  // =
  {
    TokenTree second = vec_TokenTree_extract_order(&tree.tree.vec, 0);
    Token* eq_token = token_tree_get_only_token(&second);
    assert_m(eq_token and eq_token->type is TOKEN_OPERATOR and
             is_eq_operator(eq_token->data.operator_text))
        token_tree_free(second);
  }

  // Right side
  ExprResult expr_res = expr_parse_token_tree(tree, ctx);
  CalcExprResult result;
  if (expr_res.is_ok) {
    CalcExpr to_add = {
        .type = CALC_EXPR_VARIABLE,
        .expression = expr_res.ok,
        .variable_name = var_name,
    };
    result = CalcExprOk(to_add);
  } else {
    str_free(var_name);
    result = CalcExprErr(expr_res.err_pos, expr_res.err_text);
  }
  return result;
}

static bool is_eq_tt(TokenTree* tree) {
  Token* eq_sign = token_tree_get_only_token(tree);
  if (not eq_sign or eq_sign->type is_not TOKEN_OPERATOR) return false;
  if (not is_eq_operator(eq_sign->data.operator_text)) return false;

  return true;
}

static bool is_unknown_ident(ExprContext ctx, StrSlice text) {
  return not str_slice_eq_ccp(text, "x") and not str_slice_eq_ccp(text, "y") and
         not ctx.vtable->is_function(ctx.data, text) and
         not ctx.vtable->is_variable(ctx.data, text);
}

static bool is_eq_operator(StrSlice op_text) {
  return str_slice_eq_ccp(op_text, "=") or str_slice_eq_ccp(op_text, "==");
}

// ===== FUNCTION

static bool is_function_definition(ExprContext ctx, TokenTree* tree);

static bool is_function(ExprContext ctx, TokenTree* tree) {
  // Unknown ident + idents list in brackets
  // Equality sign
  // ...Rest garbage
  if (not tree or tree->is_token or tree->tree.vec.length != 3) return false;

  return is_function_definition(ctx, &tree->tree.vec.data[0]) and
         is_eq_tt(&tree->tree.vec.data[1]);
}

static CalcExprResult parse_function(ExprContext ctx,
                                     TokenTree tree) {  // freeed
  // name, list of args and an expr
  assert_m(not tree.is_token and tree.tree.vec.length is 3);

  TokenTree right_part =
      vec_TokenTree_popget(&tree.tree.vec);  // passed ownership
  vec_TokenTree_popfree(&tree.tree.vec);     // Eq sign

  TokenTree left_part = vec_TokenTree_popget(&tree.tree.vec);  // freeed
  token_tree_free(tree);
  left_part = token_tree_unwrap_wrappers(left_part);
  assert_m(not left_part.is_token and left_part.tree.vec.length is 2);

  TokenTree args_tt = vec_TokenTree_popget(&left_part.tree.vec);  // freeed
  TokenTree name_tt = vec_TokenTree_popget(&left_part.tree.vec);  // freeed
  token_tree_free(left_part);

  str_t fn_name =
      str_slice_to_owned(token_tree_get_only_token(&name_tt)->data.ident_text);
  token_tree_free(name_tt);
  vec_str_t args = vec_str_t_create();

  assert_m(not args_tt.is_token);
  for (int i = 0; i < args_tt.tree.vec.length; i++) {
    TokenTree* item = &args_tt.tree.vec.data[i];
    if (token_tree_ttype(item) is TOKEN_COMMA) continue;
    assert_m(token_tree_ttype_skip(item) is TOKEN_IDENT);
    vec_str_t_push(
        &args,
        str_slice_to_owned(token_tree_get_only_token(item)->data.ident_text));
  }
  token_tree_free(args_tt);

  FuncConstCtx local_ctx = {
      .parent = ctx,
      .used_args = &args,
      .are_const = true,
  };

  ExprResult expr_res =
      expr_parse_token_tree(right_part, func_const_ctx_context(&local_ctx));
  if (expr_res.is_ok) {
    CalcExpr to_add = {.type = CALC_EXPR_FUNCTION,
                       .expression = expr_res.ok,
                       .function = {
                           .args = args,
                           .name = fn_name,
                       }};
    return CalcExprOk(to_add);
  } else {
    str_free(fn_name);
    vec_str_t_free(args);
    return CalcExprErr(expr_res.err_pos, expr_res.err_text);
  }
}

static bool is_function_definition(ExprContext ctx, TokenTree* tree) {
  if (not tree->is_token and tree->tree.vec.length is 1)
    return is_function_definition(ctx, &tree->tree.vec.data[0]);

  if (tree->is_token)
    return false;  // Single token is not enough to be a function definition

  vec_TokenTree* subtrees = &tree->tree.vec;

  bool is_function_pre =
      subtrees->length is 2 and                                // f(x)
      token_tree_ttype(&subtrees->data[0]) is TOKEN_IDENT and  // Name is ident
      is_unknown_ident(ctx, subtrees->data[0].token.data.ident_text);

  if (not is_function_pre) return false;

  if (subtrees->data[1].is_token) {
    // if (not is_unknown_ident(ctx, subtrees->data[1].token.data.ident_text))
    // return false;
    return false;
  } else {
    vec_TokenTree* args = &subtrees->data[1].tree.vec;
    int tokens_in_a_row = 0;
    for (int i = 0; i < args->length; i++) {
      int type = token_tree_ttype_skip(&args->data[i]);
      if (type is TOKEN_COMMA) {
        if (tokens_in_a_row is 0)
          return false;
        else
          tokens_in_a_row = 0;
      } else if (type != TOKEN_IDENT)
        return false;
      else if (tokens_in_a_row is 1)
        return false;
      else
        tokens_in_a_row++;
    }
  }

  return true;
}

// PLOT EXPR

static CalcExprResult parse_plot(ExprContext ctx, TokenTree tree) {
  ExprResult expr_res = expr_parse_token_tree(tree, ctx);

  if (expr_res.is_ok) {
    CalcExpr to_add = {
        .type = CALC_EXPR_PLOT,
        .expression = expr_res.ok,
    };
    return CalcExprOk(to_add);
  } else {
    return CalcExprErr(expr_res.err_pos, expr_res.err_text);
  }
}