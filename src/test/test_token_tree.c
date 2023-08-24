
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
  return (TtContext){
      .data = null,
      .is_function = (void*)def_is_function,
  };
}

static TokenTree parse_assert(const char* name) {
  TokenTreeResult res = token_tree_parse(name, default_ctx());
  ck_assert(res.is_ok);
  return res.ok;
}

#define EPS 0.000001

START_TEST(test_token_tree_1) {
  const char* text = "1.0";
  TokenTreeResult res = token_tree_parse(text, default_ctx());
  ck_assert(res.is_ok);
  TokenTree tree = res.ok;
  ck_assert_int_eq(token_tree_ttype_skip(&tree), TOKEN_NUMBER);
  ck_assert_int_eq(token_tree_max_len(&tree), 1);
  ck_assert_double_eq_tol(token_tree_get_only_token(&tree)->data.number_number,
                          1.0, EPS);
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

START_TEST(test_token_tree_clone) {
  TokenTree a = parse_assert("(x, y, (z, (w, a)))");
  TokenTree b = token_tree_clone(&a);
  ck_assert_int_eq(token_tree_depth(&a), token_tree_depth(&b));
  ck_assert_int_eq(token_tree_max_len(&a), token_tree_max_len(&b));
  token_tree_free(a);
  token_tree_free(b);
}
END_TEST

START_TEST(test_token_tree_err_1) {
  const char* text = ")";
  TokenTreeResult res = token_tree_parse(text, default_ctx());
  ck_assert(not res.is_ok);
  str_free(res.err.text);
}
END_TEST

START_TEST(test_token_tree_err_2) {
  const char* text = "[}";
  TokenTreeResult res = token_tree_parse(text, default_ctx());
  ck_assert(not res.is_ok);
  str_free(res.err.text);
}
END_TEST

START_TEST(test_token_tree_err_nulltk) {
  TokenTree tree = parse_assert("(1, 2)");
  ck_assert(token_tree_get_only_token(&tree) is null);
  token_tree_free(tree);
}
END_TEST

START_TEST(test_token_tree_vectors) {
  TokenTree tree = parse_assert("(([1, [[((2, 3, 4))]]] [5, 6]))");
  ck_assert_int_eq(token_tree_max_len(&tree), 5);
  token_tree_free(tree);
}
END_TEST

START_TEST(test_token_tree_print) {
  StringStream stringstream = string_stream_create();
  OutStream stream = string_stream_stream(&stringstream);

  TokenTree tree = parse_assert("(([1, [[((2, test_fn 3, 4))]]] [5, 6]))");
  token_tree_print(&tree, stream);
  token_tree_free(tree);

  str_t string = string_stream_to_str_t(stringstream);
  // ck_assert_str_eq(string.string, "(my_variable<=>x,42.000000)");
  str_free(string);
}
END_TEST

START_TEST(test_token_tree_leading_commas) {
  TokenTree tree = parse_assert(",,,1,,test_fn 1,2,,1,,1,,,");
  token_tree_free(tree);
}
END_TEST

START_TEST(test_token_tree_unwrap_wrappers) {
  TokenTree tree = parse_assert("[[[{[({1})]}]]]");
  tree = token_tree_unwrap_wrappers(tree);
  ck_assert_int_eq(token_tree_ttype_skip(&tree), TOKEN_NUMBER);
  ck_assert_int_eq(token_tree_max_len(&tree), 1);
  ck_assert_int_eq(token_tree_depth(&tree), 1);
  token_tree_free(tree);
}
END_TEST

Suite* token_tree_suite(void) {
  TCase* tc_core = tcase_create("Token tree");
  tcase_add_test(tc_core, test_token_tree_1);
  tcase_add_test(tc_core, test_token_tree_2);
  tcase_add_test(tc_core, test_token_tree_3);
  tcase_add_test(tc_core, test_token_tree_4);
  tcase_add_test(tc_core, test_token_tree_5);
  tcase_add_test(tc_core, test_token_tree_clone);
  tcase_add_test(tc_core, test_token_tree_err_1);
  tcase_add_test(tc_core, test_token_tree_err_2);
  tcase_add_test(tc_core, test_token_tree_err_nulltk);
  tcase_add_test(tc_core, test_token_tree_vectors);
  tcase_add_test(tc_core, test_token_tree_print);
  tcase_add_test(tc_core, test_token_tree_leading_commas);
  tcase_add_test(tc_core, test_token_tree_unwrap_wrappers);

  Suite* s = suite_create("Token tree suite");
  suite_add_tcase(s, tc_core);

  return s;
}
