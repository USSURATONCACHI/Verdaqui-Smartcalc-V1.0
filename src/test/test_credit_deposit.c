
#include <assert.h>
#include <check.h>
#include <math.h>

#include "../calculator/credit_deposit.h"
#include "../parser/expr.h"
#include "../util/prettify_c.h"

#define EPS 0.01

START_TEST(test_credit_annuity) {
  AnnuityResult result = calculate_annuity_credit(10, 10, 1000);
  ck_assert_double_eq_tol(result.payment_monthly, 162.745, EPS);
  ck_assert_double_eq_tol(result.total_repayment, 1627.45, EPS);
  ck_assert_double_eq_tol(result.overpayment, 627.45, EPS);
}
END_TEST

START_TEST(test_credit_diff) {
  DifferentiatedResult result = calculate_differentiated_credit(10, 10, 1000);
  ck_assert_double_eq_tol(result.first_payment, 200.0, EPS);
  ck_assert_double_eq_tol(result.last_payment, 110.0, EPS);
  ck_assert_double_eq_tol(result.total_repayment, 1550.0, EPS);
  ck_assert_double_eq_tol(result.overpayment, 550.0, EPS);
}
END_TEST

START_TEST(test_deposit) {
  vec_Placement no_placements = vec_Placement_create();

  DepositInfo info = {
      .amount = 1000.0,
      .capitalization = true,
      .capitalization_period = 1,
      .duration_months = 12,
      .interest_rate = 1.0,
      .placements = &no_placements,
      .withdrawals = &no_placements,
      .tax_rate = 0.0,
  };

  DepositResult res = calculate_deposit(info);
  ck_assert_double_eq_tol(res.total_amount, 1126.83, EPS);
  ck_assert_double_eq_tol(res.accured_interest, 0.1266 / 12.0, EPS);
  ck_assert_double_eq_tol(res.tax_sum, 0.0, EPS);

  vec_Placement_free(no_placements);
}
END_TEST

Suite *credit_deposit_suite(void) {
  TCase *tc_core = tcase_create("Credit/deposit");
  tcase_add_test(tc_core, test_credit_annuity);
  tcase_add_test(tc_core, test_credit_diff);
  tcase_add_test(tc_core, test_deposit);

  Suite *s = suite_create("Credit/deposit suite");
  suite_add_tcase(s, tc_core);

  return s;
}
