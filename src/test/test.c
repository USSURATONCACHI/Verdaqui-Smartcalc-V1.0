
#include <assert.h>
#include <check.h>
#include <math.h>

#include "../calculator/calc_backend.h"
#include "../parser/expr.h"
#include "../util/prettify_c.h"

Suite *tokenizer_suite(void);
Suite *token_tree_suite(void);
Suite *calc_backend_suite(void);
Suite *expr_value_suite(void);
Suite *backend_calcs_suite(void);
Suite *credit_deposit_suite(void);
Suite *func_const_ctx_suite(void);

typedef Suite *(*SuiteFn)();
Suite *expr_suite(void);

int main(void) {
  int number_failed = 0;
  const SuiteFn suites[] = {tokenizer_suite,     token_tree_suite,
                            calc_backend_suite,  expr_value_suite,
                            backend_calcs_suite, credit_deposit_suite,
                            func_const_ctx_suite};
  int suites_len = sizeof(suites) / sizeof(suites[0]);

  SRunner *sr = srunner_create(NULL);

  for (int i = 0; i < suites_len; i++) srunner_add_suite(sr, suites[i]());

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? 0 : 1;
}
