
#include <assert.h>
#include <check.h>
#include <math.h>

#include "../calculator/calc_backend.h"
#include "../parser/expr.h"
#include "../util/prettify_c.h"

#define EPS 0.000001
#define M_E 2.71828182846

static void add_assert_expr(CalcBackend *backend, const char *expr) {
  int old_length = backend->expressions.length;

  str_free(calc_backend_add_expr(backend, expr));

  ck_assert(old_length < backend->expressions.length);
}

#define Slice(text) \
  (StrSlice) { .start = (text), .length = strlen(text) }

START_TEST(test_cbc_function_usage_1) {
  CalcBackend backend = calc_backend_create();
  ExprContext ctx = calc_backend_get_context(&backend);

  add_assert_expr(&backend, "w(x) = x*e^x");
  add_assert_expr(&backend, "a = 150 * w(4)");

  ExprValueResult value = ctx.vtable->get_variable_val(ctx.data, Slice("a"));
  ck_assert(value.is_ok);
  ck_assert(value.ok.type is EXPR_VALUE_NUMBER);
  ck_assert_double_eq_tol(value.ok.number, 150.0 * (4.0 * pow(M_E, 4.0)),
                          0.001);

  ExprValue clone = expr_value_clone(&value.ok);
  ck_assert_int_eq(value.ok.type, clone.type);
  ck_assert_double_eq(value.ok.number, clone.number);

  expr_value_free(value.ok);
  expr_value_free(clone);

  calc_backend_free(backend);
}
END_TEST

static void print_last(CalcBackend *backend) {
  CalcExpr *last = calc_backend_last_expr(backend);
  ck_assert(last);
  calc_expr_print(last, DEBUG_OUT);
}

START_TEST(test_cbc_function_usage_2) {
  CalcBackend backend = calc_backend_create();
  ExprContext ctx = calc_backend_get_context(&backend);

  add_assert_expr(&backend, "w(x) = x * e^x");
  print_last(&backend);
  add_assert_expr(&backend, "a = 10 * w(2)");
  print_last(&backend);
  add_assert_expr(&backend, "b = 15 - w(e)");
  print_last(&backend);
  add_assert_expr(&backend, "c = sin(3) + a + b");
  print_last(&backend);

  // Value
  double must_be = sin(3.0) + (10.0 * (2.0 * pow(M_E, 2.0))) +
                   (15.0 - (M_E * pow(M_E, M_E)));

  ExprValueResult value = ctx.vtable->get_variable_val(ctx.data, Slice("c"));
  ck_assert(value.is_ok);
  ck_assert(value.ok.type is EXPR_VALUE_NUMBER);
  ck_assert_double_eq_tol(value.ok.number, must_be, 0.001);
  expr_value_free(value.ok);

  // Clone 1
  CalcExpr *w_function = calc_backend_get_function(&backend, "w");
  ck_assert(w_function);
  ck_assert_int_eq(w_function->type, CALC_EXPR_FUNCTION);
  CalcExpr w_clone = calc_expr_clone(w_function);
  ck_assert_int_eq(w_function->type, w_clone.type);
  ck_assert_str_eq(w_function->function.name.string,
                   w_clone.function.name.string);
  calc_expr_free(w_clone);

  // Clone 2
  CalcExpr *b_variable = calc_backend_get_variable(&backend, "b");
  ck_assert(b_variable);
  ck_assert_int_eq(b_variable->type, CALC_EXPR_VARIABLE);
  CalcExpr b_clone = calc_expr_clone(b_variable);
  ck_assert_int_eq(b_variable->type, b_clone.type);
  ck_assert_str_eq(b_variable->variable_name.string,
                   b_clone.variable_name.string);
  calc_expr_free(b_clone);

  calc_backend_free(backend);
}
END_TEST

#define TYPE_ERR -1

#define CBC_TEST_TYPE(num, expr, must_have_type)       \
  START_TEST(test_cbc_function_##num) {                \
    CalcBackend backend = calc_backend_create();       \
                                                       \
    int old_length = backend.expressions.length;       \
    str_free(calc_backend_add_expr(&backend, (expr))); \
    int type = TYPE_ERR;                               \
    if (old_length < backend.expressions.length)       \
      type = calc_backend_last_expr(&backend)->type;   \
    ck_assert_int_eq(type, (must_have_type));          \
                                                       \
    calc_backend_free(backend);                        \
  }                                                    \
  END_TEST

CBC_TEST_TYPE(forbidden_symbols, "1 + $", TYPE_ERR)
CBC_TEST_TYPE(actions, "a += 1", TYPE_ERR)
CBC_TEST_TYPE(var_err, "a = 1232323 +", TYPE_ERR)
CBC_TEST_TYPE(pseudo_func, "a a = 1232323 + x", CALC_EXPR_PLOT)

START_TEST(test_cbc_values) {
  CalcBackend backend = calc_backend_create();

  ck_assert(calc_backend_is_var_const(&backend, "e"));
  CalcValue *val = calc_backend_get_value(&backend, "e");
  ck_assert(val);
  calc_value_print(val, DEBUG_OUT);
  ck_assert_int_eq(val->value.type, EXPR_VALUE_NUMBER);
  ck_assert_double_eq_tol(val->value.number, M_E, EPS);

  calc_backend_free(backend);
}
END_TEST

START_TEST(test_expr_info) {
  CalcBackend backend = calc_backend_create();

  add_assert_expr(&backend, "a = 12 + 3");
  CalcExpr *var = calc_backend_last_expr(&backend);
  ck_assert(var);
  int type = calc_backend_get_expr_type(&backend, &var->expression);
  ck_assert(type is VALUE_TYPE_UNKNOWN or type is EXPR_VALUE_NUMBER);

  ExprContext ctx = calc_backend_get_context(&backend);
  ExprVariableInfo info = ctx.vtable->get_variable_info(ctx.data, Slice("a"));
  ck_assert(info.is_const);
  ck_assert(info.expression);
  ck_assert(info.correct_context.vtable);
  ck_assert(info.correct_context.data is ctx.data);

  calc_backend_free(backend);
}
END_TEST

START_TEST(test_calculate_xy) {
  CalcBackend backend = calc_backend_create();

  ExprValueResult res = calc_calculate_expr("x + y + e + sin 0", 1.0, 2.0);
  ck_assert(res.is_ok);
  ck_assert_int_eq(res.ok.type, EXPR_VALUE_NUMBER);
  ck_assert_double_eq_tol(res.ok.number, 3.0 + M_E, EPS);
  expr_value_free(res.ok);

  calc_backend_free(backend);
}
END_TEST

START_TEST(test_calculate_funcs) {
  CalcBackend backend = calc_backend_create();

  ExprValueResult res = calc_calculate_expr(
      "sin 0 + cos(pi / 2) + tan(0) + acos(1) + asin(0) + atan(0) + sqrt(0) + "
      "ln(1) + log(1)",
      1.0, 2.0);
  ck_assert(res.is_ok);
  ck_assert_int_eq(res.ok.type, EXPR_VALUE_NUMBER);
  ck_assert_double_eq_tol(res.ok.number, 0.0, EPS);
  expr_value_free(res.ok);

  calc_backend_free(backend);
}
END_TEST
START_TEST(test_calculate_funcs2) {
  CalcBackend backend = calc_backend_create();

  ExprValueResult res = calc_calculate_expr(
      "min(join(sin(1, 2, -pi/2), [2, 1, -1])) + max(slice(1..4, 1..2))", 1.0,
      2.0);
  ck_assert(res.is_ok);
  ck_assert_int_eq(res.ok.type, EXPR_VALUE_NUMBER);
  ck_assert_double_eq_tol(res.ok.number, 1.0, EPS);
  x_printf("Value is: %$expr_value", res.ok);
  expr_value_free(res.ok);

  calc_backend_free(backend);
}
END_TEST

START_TEST(test_calculate_funcs3) {
  ExprValueResult res = calc_calculate_expr(
      "min([1, 2, 3], [0, 1, 2]) + max ([-3, -2, -1], [0, -1, -2])", 1.0, 2.0);
  ck_assert(res.is_ok);
  ck_assert_int_eq(res.ok.type, EXPR_VALUE_NUMBER);
  ck_assert_double_eq_tol(res.ok.number, 0.0, EPS);
  expr_value_free(res.ok);
}
END_TEST
// Get expr type
// Get variable info
//

Suite *backend_calcs_suite(void) {
  TCase *tc_core = tcase_create("Backend calculations");
  tcase_add_test(tc_core, test_cbc_function_usage_1);
  tcase_add_test(tc_core, test_cbc_function_usage_2);

  tcase_add_test(tc_core, test_cbc_function_forbidden_symbols);
  tcase_add_test(tc_core, test_cbc_function_actions);
  tcase_add_test(tc_core, test_cbc_function_var_err);
  tcase_add_test(tc_core, test_cbc_function_pseudo_func);

  tcase_add_test(tc_core, test_cbc_values);
  tcase_add_test(tc_core, test_expr_info);
  tcase_add_test(tc_core, test_calculate_xy);
  tcase_add_test(tc_core, test_calculate_funcs);
  tcase_add_test(tc_core, test_calculate_funcs2);
  tcase_add_test(tc_core, test_calculate_funcs3);

  Suite *s = suite_create("CalcBackend calculations suite");
  suite_add_tcase(s, tc_core);

  return s;
}
// join, slice, min, max,