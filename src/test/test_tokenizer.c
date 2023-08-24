
#include <assert.h>
#include <check.h>
#include <math.h>
#include <string.h>

#include "../calculator/calc_backend.h"
#include "../parser/expr.h"
#include "../util/prettify_c.h"

static int tokens_count(const char *text) {
  int count = 0;

  TokenResult res = tk_next_token(text);
  while (res.has_token) {
    count++;
    res = tk_next_token(res.next_token_pos);
  }

  return count;
}

START_TEST(test_tokenize_one_token) {
  const char *const examples[] = {
      "1",    "1.0",    "1e2", "1.0e2",    "-1",           "-1.0",
      "-1e2", "-1.0e2", "- 1", "- 1.0",    "- 1e2",        "- 1.0e2",
      "sin",  "x",      "y",   "xadtgjip", "   snsinsin ", "(",
      ",",    ".",      ")",   "[",        "3941491e10",   "*",
      "+=",   "-=",     "==",  "1.",       "123."};

  for (int i = 0; i < (int)LEN(examples); i++)
    ck_assert(tokens_count(examples[i]) is 1);
}
END_TEST

START_TEST(test_tokenize_two_tokens) {
  const char *const examples[] = {"1x",           "1.0 y",    "1e2 cum",
                                  "1.0e2 foobar", "sin -1",   "token      -1.0",
                                  "* -1e2",       "+ -1.0e2", ", - 1",
                                  "( - 1.0",      ") - 1e2",  "x x",
                                  "++",           "=+",       "= =",
                                  "( )",          "15..",     "15ea"};

  for (int i = 0; i < (int)LEN(examples); i++)
    ck_assert_int_eq(tokens_count(examples[i]), 2);
}
END_TEST

START_TEST(test_tokenize_three_tokens) {
  const char *const examples[] = {
      "x 1x",       "( 1.0 y", "] 1e2 cum", ", 1.0e2 foobar", ", , -1",
      "    (((   ", "*)-",     "======",    "!=,=",           "1.0 1.0 1.0",
      "sin sin in", "x y yx",  "( z )"};

  for (int i = 0; i < (int)LEN(examples); i++)
    ck_assert(tokens_count(examples[i]) is 3);
}
END_TEST

START_TEST(test_token_types_texts) {
  ck_assert_str_eq(token_type_text(TOKEN_IDENT), "Ident");
  ck_assert_str_eq(token_type_text(TOKEN_COMMA), "Comma");
  ck_assert_str_eq(token_type_text(TOKEN_BRACKET), "Bracket");
  ck_assert_str_eq(token_type_text(TOKEN_NUMBER), "Number");
  ck_assert_str_eq(token_type_text(TOKEN_OPERATOR), "Operator");
}
END_TEST

START_TEST(test_tokenizer_symbols) {
  ck_assert(tk_is_symbol_allowed('a'));
  ck_assert(tk_is_symbol_allowed('1'));
  ck_assert(tk_is_symbol_allowed('.'));
  ck_assert(tk_is_symbol_allowed(','));
  ck_assert(tk_is_symbol_allowed('F'));
  ck_assert(tk_is_symbol_allowed('('));
  ck_assert(tk_is_symbol_allowed(']'));
  ck_assert(tk_is_symbol_allowed('e'));
  ck_assert(tk_is_symbol_allowed('%'));
  ck_assert(tk_is_symbol_allowed('-'));
  ck_assert(tk_is_symbol_allowed('='));
  ck_assert(tk_is_symbol_allowed('^'));
  ck_assert(not tk_is_symbol_allowed('$'));
  ck_assert(not tk_is_symbol_allowed('#'));
  ck_assert(not tk_is_symbol_allowed('&'));
}
END_TEST

#define TokenIdent(literal)                   \
  (Token) {                                   \
    .type = TOKEN_IDENT, .data.ident_text = { \
      .start = (literal),                     \
      .length = strlen(literal)               \
    }                                         \
  }
#define TokenBracket(bracket) \
  (Token) { .type = TOKEN_BRACKET, .data.bracket_symbol = (bracket) }
#define TokenNumber(num) \
  (Token) { .type = TOKEN_NUMBER, .data.number_number = (num) }
#define TokenOperator(literal)                      \
  (Token) {                                         \
    .type = TOKEN_OPERATOR, .data.operator_text = { \
      .start = (literal),                           \
      .length = strlen(literal)                     \
    }                                               \
  }
#define TokenComma \
  (Token) { .type = TOKEN_COMMA }

START_TEST(test_token_print_ident) {
  StringStream stringstream = string_stream_create();
  OutStream stream = string_stream_stream(&stringstream);

  token_print(&TokenBracket('('), stream);
  token_print(&TokenIdent("my_variable"), stream);
  token_print(&TokenOperator("="), stream);
  token_print(&TokenIdent("x"), stream);
  token_print(&TokenComma, stream);
  token_print(&TokenNumber(42.0), stream);
  token_print(&TokenBracket(')'), stream);

  str_t string = string_stream_to_str_t(stringstream);
  // ck_assert_str_eq(string.string, "(my_variable<=>x,42.000000)");
  str_free(string);
}
END_TEST

Suite *tokenizer_suite(void) {
  TCase *tc_core = tcase_create("Tokenizer");
  tcase_add_test(tc_core, test_tokenize_one_token);
  tcase_add_test(tc_core, test_tokenize_two_tokens);
  tcase_add_test(tc_core, test_tokenize_three_tokens);
  tcase_add_test(tc_core, test_token_types_texts);
  tcase_add_test(tc_core, test_token_print_ident);
  tcase_add_test(tc_core, test_tokenizer_symbols);

  Suite *s = suite_create("Tokenizer suite");
  suite_add_tcase(s, tc_core);

  return s;
}
