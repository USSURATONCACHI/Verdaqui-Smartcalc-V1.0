
#include <assert.h>
#include <check.h>
#include <math.h>

#include "../calculator/calc_backend.h"
#include "../parser/expr.h"
#include "../util/prettify_c.h"

#define EPS 0.000001

START_TEST(test_cb_creation) {
  CalcBackend backend = calc_backend_create();
  ExprContext ctx = calc_backend_get_context(&backend);

  ck_assert(ctx.data);
  ck_assert(ctx.vtable);
  ck_assert(ctx.vtable->call_function);
  ck_assert(ctx.vtable->get_variable_val);
  ck_assert(ctx.vtable->is_function);

  calc_backend_free(backend);
}
END_TEST

START_TEST(test_cb_expr_parse_1) {
  CalcBackend backend = calc_backend_create();
  ExprContext ctx = calc_backend_get_context(&backend);

  ExprResult res = expr_parse_string("1.0", ctx);
  ck_assert(res.is_ok);
  Expr expr = res.ok;
  ck_assert_int_eq(expr.type, EXPR_NUMBER);
  ck_assert_double_eq_tol(expr.number.value, 1.0, EPS);

  expr_free(expr);

  calc_backend_free(backend);
}
END_TEST

START_TEST(test_cb_expr_parse_2) {
  CalcBackend backend = calc_backend_create();
  ExprContext ctx = calc_backend_get_context(&backend);

  ExprResult res = expr_parse_string(
      "sin cos tan (x y) = sin cos tan x + sin cos tan y * 1.0", ctx);
  ck_assert(res.is_ok);
  Expr expr = res.ok;
  ck_assert(not calc_backend_is_expr_const(&backend, &expr));
  expr_free(expr);

  calc_backend_free(backend);
}
END_TEST

//
//

#define CB_EXPR_CALC_TEST(num, text)                                  \
  START_TEST(test_cb_expr_parse_##num) {                              \
    CalcBackend backend = calc_backend_create();                      \
    ExprContext ctx = calc_backend_get_context(&backend);             \
    ExprResult res = expr_parse_string((text), ctx);                  \
    ck_assert(res.is_ok);                                             \
    Expr expr = res.ok;                                               \
    Expr clone = expr_clone(&expr);                                   \
    expr_free(clone);                                                 \
    debugln("Expr: %$expr", expr);                                    \
    ck_assert(calc_backend_is_expr_const(&backend, &expr));           \
    ExprValueResult val_res = expr_calculate(&expr, ctx);             \
    if (not val_res.is_ok) panic("Err: %s", val_res.err_text.string); \
                                                                      \
    ck_assert(val_res.is_ok);                                         \
                                                                      \
    expr_value_free(val_res.ok);                                      \
    expr_free(expr);                                                  \
                                                                      \
    calc_backend_free(backend);                                       \
  }                                                                   \
  END_TEST

START_TEST(test_cb_expr_function) {
  CalcBackend backend = calc_backend_create();
  ExprContext ctx = calc_backend_get_context(&backend);
  ExprResult res = expr_parse_string("cos sin sin sin 0", ctx);
  ck_assert(res.is_ok);
  Expr expr = res.ok;
  Expr clone = expr_clone(&expr);
  expr_free(clone);
  debugln("Expr: %$expr", expr);

  ck_assert(calc_backend_is_expr_const(&backend, &expr));
  ExprValueResult val_res = expr_calculate(&expr, ctx);
  if (not val_res.is_ok) panic("Err: %s", val_res.err_text.string);
  ck_assert(val_res.is_ok);
  ck_assert_int_eq(val_res.ok.type, EXPR_VALUE_NUMBER);
  ck_assert_double_eq_tol(val_res.ok.number, 1.0, EPS);

  expr_value_free(val_res.ok);
  expr_free(expr);

  calc_backend_free(backend);
}
END_TEST

CB_EXPR_CALC_TEST(
    3,
    "[1.0, [2..=5][0], [3..6][0], pi, e][2] * e + 3 - 4 * 5 / 6 ^ 7 mod 8 % 9 ")
CB_EXPR_CALC_TEST(4,
                  "((((((([0..5] + [0..=4]) * [0..5]) - [0..5]) / [1..=5]) % "
                  "[1..=5]) ^ [0..5])[0]) := 1 += 2 *= 3 /= 4 %= 5 -= 6 ^= 7")
CB_EXPR_CALC_TEST(5, "(((((([1, 2, 3] + 4) - 5) * 6) / 7) % 8) ^ 9)")
CB_EXPR_CALC_TEST(6, "((((((1 = 2) != 3) < 4) > 5) <= 6) >= 7)")
CB_EXPR_CALC_TEST(7, "[1, 2, 3] = [4, 5, 6]")
CB_EXPR_CALC_TEST(8, "[1, 2, 3] > [4, 3, 2]")
CB_EXPR_CALC_TEST(9, "[1, 2, 3] < [4, 3, 2]")
CB_EXPR_CALC_TEST(10, "[1, 2, 3] >= [4, 3, 2]")
CB_EXPR_CALC_TEST(11, "[1, 2, 3] <= [4, 3, 2]")
CB_EXPR_CALC_TEST(12, "1 + [1, 2, 3]")

// Unary ops
// Function as ident
// Bracket as ident ?
// Comma as ident ?
// Operator only
// More binops

#define CB_ERR_TEST(num, text)                            \
  START_TEST(test_cb_expr_err_##num) {                    \
    CalcBackend backend = calc_backend_create();          \
    ExprContext ctx = calc_backend_get_context(&backend); \
                                                          \
    ExprResult res = expr_parse_string((text), ctx);      \
    ck_assert(not res.is_ok);                             \
                                                          \
    str_free(res.err_text);                               \
                                                          \
    calc_backend_free(backend);                           \
  }                                                       \
  END_TEST

CB_ERR_TEST(1, ")")
CB_ERR_TEST(2, "sin ** 2")
CB_ERR_TEST(3, ",,,,,,")
CB_ERR_TEST(4, "")
CB_ERR_TEST(5, "x 2 y")
CB_ERR_TEST(6, "+")
CB_ERR_TEST(7, "1 +")
CB_ERR_TEST(8, "sin")

START_TEST(test_cb_clone) {
  CalcBackend backend = calc_backend_create();

  CalcBackend backend2 = calc_backend_clone(&backend);
  calc_backend_free(backend2);

  calc_backend_free(backend);
}
END_TEST

Suite *calc_backend_suite(void) {
  TCase *tc_core = tcase_create("Calc Backend");
  tcase_add_test(tc_core, test_cb_creation);
  tcase_add_test(tc_core, test_cb_expr_parse_1);
  tcase_add_test(tc_core, test_cb_expr_parse_2);
  tcase_add_test(tc_core, test_cb_expr_parse_3);
  tcase_add_test(tc_core, test_cb_expr_parse_4);
  tcase_add_test(tc_core, test_cb_expr_parse_5);
  tcase_add_test(tc_core, test_cb_expr_parse_6);
  tcase_add_test(tc_core, test_cb_expr_parse_7);
  tcase_add_test(tc_core, test_cb_expr_parse_8);
  tcase_add_test(tc_core, test_cb_expr_parse_9);
  tcase_add_test(tc_core, test_cb_expr_parse_10);
  tcase_add_test(tc_core, test_cb_expr_parse_11);
  tcase_add_test(tc_core, test_cb_expr_parse_12);
  tcase_add_test(tc_core, test_cb_expr_function);

  tcase_add_test(tc_core, test_cb_expr_err_1);
  tcase_add_test(tc_core, test_cb_expr_err_2);
  tcase_add_test(tc_core, test_cb_expr_err_3);
  tcase_add_test(tc_core, test_cb_expr_err_4);
  tcase_add_test(tc_core, test_cb_expr_err_5);
  tcase_add_test(tc_core, test_cb_expr_err_6);
  tcase_add_test(tc_core, test_cb_expr_err_7);
  tcase_add_test(tc_core, test_cb_expr_err_8);

  tcase_add_test(tc_core, test_cb_clone);

  Suite *s = suite_create("Calc Backend suite");
  suite_add_tcase(s, tc_core);

  return s;
}
