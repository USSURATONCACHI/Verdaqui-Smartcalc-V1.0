
#include <assert.h>
#include <check.h>
#include <math.h>

#include "../calculator/calc_backend.h"
#include "../parser/expr.h"
#include "../util/prettify_c.h"

#define EPS 0.000001

START_TEST(test_expr_value_free) {
  ExprValue val_none = {.type = EXPR_VALUE_NONE};
  ExprValue val_num = {.type = EXPR_VALUE_NUMBER, .number = 42.0};
  ExprValue arr[] = {val_num, val_none};
  ExprValue val_vec = {.type = EXPR_VALUE_VEC,
                       .vec = vec_ExprValue_create_copy(arr, 2)};

  expr_value_free(val_none);
  expr_value_free(val_num);
  expr_value_free(val_vec);
}
END_TEST

START_TEST(test_expr_value_clone) {
  ExprValue val_none = {.type = EXPR_VALUE_NONE};
  ExprValue val_num = {.type = EXPR_VALUE_NUMBER, .number = 42.0};
  ExprValue arr[] = {val_num, val_none};
  ExprValue val_vec = {.type = EXPR_VALUE_VEC,
                       .vec = vec_ExprValue_create_copy(arr, 2)};

  ExprValue copy = expr_value_clone(&val_none);
  expr_value_free(copy);
  copy = expr_value_clone(&val_num);
  expr_value_free(copy);
  copy = expr_value_clone(&val_vec);
  expr_value_free(copy);

  expr_value_free(val_none);
  expr_value_free(val_num);
  expr_value_free(val_vec);
}
END_TEST

START_TEST(test_expr_value_print) {
  StringStream stringstream = string_stream_create();
  OutStream os = string_stream_stream(&stringstream);

  ExprValue val_none = {.type = EXPR_VALUE_NONE};
  ExprValue val_num = {.type = EXPR_VALUE_NUMBER, .number = 42.0};
  ExprValue arr[] = {val_num, val_none};
  ExprValue val_vec = {.type = EXPR_VALUE_VEC,
                       .vec = vec_ExprValue_create_copy(arr, 2)};

  x_sprintf(os, "%$expr_value %$expr_value %$expr_value", val_none, val_num,
            val_vec);
  string_stream_free(stringstream);

  expr_value_free(val_none);
  expr_value_free(val_num);
  expr_value_free(val_vec);
}
END_TEST

START_TEST(test_expr_value_type_text) {
  ExprValue val_none = {.type = EXPR_VALUE_NONE};
  ExprValue val_num = {.type = EXPR_VALUE_NUMBER, .number = 42.0};
  ExprValue arr[] = {val_num, val_none};
  ExprValue val_vec = {.type = EXPR_VALUE_VEC,
                       .vec = vec_ExprValue_create_copy(arr, 2)};

  ck_assert_str_eq(expr_value_type_text(val_none.type), "None");
  ck_assert_str_eq(expr_value_type_text(val_num.type), "Number");
  ck_assert_str_eq(expr_value_type_text(val_vec.type), "Vec");

  expr_value_free(val_none);
  expr_value_free(val_num);
  expr_value_free(val_vec);
}
END_TEST

Suite *expr_value_suite(void) {
  TCase *tc_core = tcase_create("ExprValue");
  tcase_add_test(tc_core, test_expr_value_free);
  tcase_add_test(tc_core, test_expr_value_clone);
  tcase_add_test(tc_core, test_expr_value_print);
  tcase_add_test(tc_core, test_expr_value_type_text);

  Suite *s = suite_create("ExprValue suite");
  suite_add_tcase(s, tc_core);

  return s;
}
