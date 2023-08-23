
#include <assert.h>
#include <check.h>
#include <math.h>

#include "../calculator/calc_backend.h"
#include "../parser/expr.h"
#include "../util/prettify_c.h"

static int token_tree_depth(TokenTree* tree) {
    if (tree->is_token) {
        return 1;
    } else {
        int max = 0;
        for (int i = 0; i < tree->tree.vec.length; i++) {
            int cur_depth = 1 + token_tree_depth(&tree->tree.vec.data[i]);
            max = max > cur_depth ? max : cur_depth;
        }
        return max;
    }
}

static int token_tree_max_len(TokenTree* tree) {
    if (tree->is_token) {
        return 1;
    } else {
        int max = tree->tree.vec.length;
        for (int i = 0; i < tree->tree.vec.length; i++) {
            int cur_len = token_tree_max_len(&tree->tree.vec.data[i]);
            max = max > cur_len ? max : cur_len;
        }
        return max;
    }
}

static bool def_is_function(void* data, StrSlice name) {
    unused(data);
    return str_slice_eq_ccp(name, "test_fn");
}

static TtContext default_ctx() {
    return (TtContext) {
        .data = null,
        .is_function = (void*)def_is_function,
    };
}

#define EPS 0.000001

START_TEST(test_token_tree_1) {
    const char* text = "1.0";
    TokenTreeResult res = token_tree_parse(text, default_ctx());
    ck_assert(res.is_ok);
    TokenTree tree = res.ok;
    ck_assert_int_eq(token_tree_ttype_skip(&tree), TOKEN_NUMBER);
    ck_assert_int_eq(token_tree_max_len(&tree), 1);
    ck_assert_double_eq_tol(token_tree_get_only_token(&tree)->data.number_number, 1.0, EPS);
    token_tree_free(tree);
}
END_TEST


START_TEST(test_token_tree_2) {
    const char* text = "x";
    TokenTreeResult res = token_tree_parse(text, default_ctx());
    ck_assert(res.is_ok);
    TokenTree tree = res.ok;
    ck_assert_int_eq(token_tree_ttype_skip(&tree), TOKEN_IDENT);
    ck_assert_int_eq(token_tree_max_len(&tree), 1);
    token_tree_free(tree);
}
END_TEST

START_TEST(test_token_tree_3) {
    const char* text = "((((x))))";
    TokenTreeResult res = token_tree_parse(text, default_ctx());
    ck_assert(res.is_ok);
    TokenTree tree = res.ok;
    ck_assert_int_eq(token_tree_ttype_skip(&tree), TOKEN_IDENT);
    ck_assert_int_eq(token_tree_max_len(&tree), 1);
    token_tree_free(tree);
}
END_TEST

START_TEST(test_token_tree_4) {
    const char* text = "1, 23";
    TokenTreeResult res = token_tree_parse(text, default_ctx());
    ck_assert(res.is_ok);
    TokenTree tree = res.ok;
    ck_assert_int_eq(token_tree_ttype_skip(&tree), -1);
    ck_assert_int_eq(token_tree_max_len(&tree), 3);
    token_tree_free(tree);
}
END_TEST

START_TEST(test_token_tree_5) {
    const char* text = "+";
    TokenTreeResult res = token_tree_parse(text, default_ctx());
    ck_assert(res.is_ok);
    TokenTree tree = res.ok;
    ck_assert_int_eq(token_tree_ttype_skip(&tree), TOKEN_OPERATOR);
    ck_assert_int_eq(token_tree_max_len(&tree), 1);
    token_tree_free(tree);
}
END_TEST


START_TEST(test_token_tree_err_1) {
    const char* text = ")";
    TokenTreeResult res = token_tree_parse(text, default_ctx());
    ck_assert(not res.is_ok);
    str_free(res.err.text);
}
END_TEST

Suite *token_tree_suite(void) {
  TCase *tc_core = tcase_create("Token tree");
  tcase_add_test(tc_core, test_token_tree_1);
  tcase_add_test(tc_core, test_token_tree_2);
  tcase_add_test(tc_core, test_token_tree_3);
  tcase_add_test(tc_core, test_token_tree_4);
  tcase_add_test(tc_core, test_token_tree_5);
  tcase_add_test(tc_core, test_token_tree_err_1);

  Suite *s = suite_create("Token tree suite");
  suite_add_tcase(s, tc_core);

  return s;
}
