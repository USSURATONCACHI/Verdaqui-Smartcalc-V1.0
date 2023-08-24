
#include <assert.h>
#include <check.h>
#include <math.h>

#include "../calculator/calc_backend.h"
#include "../calculator/func_const_ctx.h"
#include "../parser/expr.h"
#include "../util/prettify_c.h"

#define EPS 0.000001
#define M_E 2.71828182846

#define Slice(text) \
  (StrSlice) { .start = (text), .length = strlen(text) }

#define DEFAULT_FCTX_INIT                                   \
  CalcBackend backend = calc_backend_create();              \
  ExprContext top_ctx = calc_backend_get_context(&backend); \
                                                            \
  vec_str_t used_args = vec_str_t_create();                 \
  vec_str_t_push(&used_args, str_literal("a"));             \
  vec_str_t_push(&used_args, str_literal("b"));             \
  vec_str_t_push(&used_args, str_literal("c"));             \
                                                            \
  FuncConstCtx f_ctx = {                                    \
      .are_const = true,                                    \
      .parent = top_ctx,                                    \
      .used_args = &used_args,                              \
  };                                                        \
  ExprContext ctx = func_const_ctx_context(&f_ctx);

START_TEST(test_fctx_1) {
  DEFAULT_FCTX_INIT

  ck_assert(ctx.vtable->is_variable(ctx.data, Slice("a")));
  ck_assert(ctx.vtable->is_variable(ctx.data, Slice("e")));
  ck_assert(ctx.vtable->is_function(ctx.data, Slice("sin")));
  ck_assert(not ctx.vtable->is_function(ctx.data, Slice("a")));

  vec_str_t_free(used_args);
  calc_backend_free(backend);
}
END_TEST

START_TEST(test_fctx_2) {
  DEFAULT_FCTX_INIT

  ExprValueResult val = ctx.vtable->get_variable_val(ctx.data, Slice("e"));
  ck_assert(val.is_ok);
  expr_value_free(val.ok);

  vec_str_t_free(used_args);
  calc_backend_free(backend);
}
END_TEST

START_TEST(test_fctx_3) {
  DEFAULT_FCTX_INIT

  ExprResult res = expr_parse_string("[sin(e), sin(pi), cos(0)]", ctx);
  ck_assert(res.is_ok);

  ck_assert(ctx.vtable->is_expr_const(ctx.data, &res.ok));
  ExprValueResult val = expr_calculate(&res.ok, ctx);
  ck_assert(val.is_ok);

  expr_value_free(val.ok);
  expr_free(res.ok);

  vec_str_t_free(used_args);
  calc_backend_free(backend);
}
END_TEST

START_TEST(test_fctx_4) {
  DEFAULT_FCTX_INIT

  ExprVariableInfo info = ctx.vtable->get_variable_info(ctx.data, Slice("a"));
  ck_assert(info.is_const);

  vec_str_t_free(used_args);
  calc_backend_free(backend);
}
END_TEST

START_TEST(test_fctx_5) {
  DEFAULT_FCTX_INIT

  ExprVariableInfo info = ctx.vtable->get_variable_info(ctx.data, Slice("x"));
  ck_assert(not info.is_const);

  vec_str_t_free(used_args);
  calc_backend_free(backend);
}
END_TEST

START_TEST(test_fctx_6) {
  DEFAULT_FCTX_INIT

  ExprFunctionInfo info = ctx.vtable->get_function_info(ctx.data, Slice("sin"));
  ck_assert(not info.is_const);

  vec_str_t_free(used_args);
  calc_backend_free(backend);
}
END_TEST

START_TEST(test_fctx_7) {
  DEFAULT_FCTX_INIT

  ExprFunctionInfo info =
      ctx.vtable->get_function_info(ctx.data, Slice("noop"));
  ck_assert(not info.is_const);

  vec_str_t_free(used_args);
  calc_backend_free(backend);
}
END_TEST

Suite *func_const_ctx_suite(void) {
  TCase *tc_core = tcase_create("FuncConstCtx");
  tcase_add_test(tc_core, test_fctx_1);
  tcase_add_test(tc_core, test_fctx_2);
  tcase_add_test(tc_core, test_fctx_3);
  tcase_add_test(tc_core, test_fctx_4);
  tcase_add_test(tc_core, test_fctx_5);
  tcase_add_test(tc_core, test_fctx_6);
  tcase_add_test(tc_core, test_fctx_7);

  Suite *s = suite_create("FuncConstCtx suite");
  suite_add_tcase(s, tc_core);

  return s;
}
// join, slice, min, max,