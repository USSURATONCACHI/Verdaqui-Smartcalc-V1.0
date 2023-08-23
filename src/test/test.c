
#include <assert.h>
#include <check.h>
#include <math.h>

#include "../calculator/calc_backend.h"
#include "../parser/expr.h"
#include "../util/prettify_c.h"

Suite *tokenizer_suite(void);
Suite *token_tree_suite(void);

typedef Suite *(*SuiteFn)();

int main(void) {
  int number_failed = 0;
  const SuiteFn suites[] = {tokenizer_suite, token_tree_suite};
  int suites_len = sizeof(suites) / sizeof(suites[0]);

  SRunner *sr = srunner_create(NULL);

  for (int i = 0; i < suites_len; i++) srunner_add_suite(sr, suites[i]());

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? 0 : 1;
}

