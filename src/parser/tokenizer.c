// =====
// HEADERS
#include "tokenizer.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "../util/allocator.h"
#include "../util/prettify_c.h"

#define VECTOR_C Token
#undef VECTOR_ITEM_DESTRUCTOR
#include "../util/vector.h"  // vec_Token

// =====
// HELPER FUNCTIONS
static bool is_letter(char c);
static bool is_digit(char c);
static bool is_bracket(char c);
static char next_char_skip_spaces(const char* text);

// =====
// =
// = tk_next_token
// =
// =====

// >-<helper functions>-<
static const char* skip_spaces(const char* string);
static bool check_operator(const char* string, struct TokenResult* result);

static TokenResult scan_comma(const char* string, TokenResult result);
static TokenResult scan_bracket(const char* string, TokenResult result);
static TokenResult scan_ident(const char* string, TokenResult result);
static TokenResult scan_number(const char* string, TokenResult result);
static double read_num_non_scientific(const char* string, int* offset);
static bool has_double_dot_after_digits(const char* string);

// >-<function itself>-<
struct TokenResult tk_next_token(const char* string) {
  // ==== 1. Skip spaces
  string = skip_spaces(string);

  struct TokenResult result = {
      .has_token = false,
      .token = (Token){.type = TOKEN_NUMBER,
                       .start_pos = string,
                       .data.number_number = 0.0},
      .next_token_pos = &string[1],  // Next symbol by default
  };
  if (not string or string[0] is '\0') {
    result.next_token_pos = null;
    return result;
  }

  if (string[0] is ',') return scan_comma(string, result);

  if (is_bracket(string[0])) return scan_bracket(string, result);

  // ==== 4. Check if it is negative number
  bool is_number = false;
  char next_char = next_char_skip_spaces(&string[1]);
  if (string[0] is '-' and (is_digit(next_char) or next_char is '.'))
    is_number = true;

  // ==== 5. Scan for operators
  if (not is_number and check_operator(string, &result)) return result;

  if (is_letter(string[0])) {
    // ==== 6. Check for ident
    return scan_ident(string, result);
  } else if (is_number or is_digit(string[0]) or string[0] is '.') {
    // ==== 7. Check for number
    return scan_number(string, result);
  } else {
    panic("Unsupported symbols in tokenizer");
  }

  return result;
}

static TokenResult scan_comma(const char* string, TokenResult result) {
  unused(string);
  result.has_token = true;
  result.token.type = TOKEN_COMMA;
  return result;
}

static TokenResult scan_bracket(const char* string, TokenResult result) {
  result.has_token = true;
  result.token.type = TOKEN_BRACKET;
  result.token.data.bracket_symbol = string[0];
  return result;
}

// =====
// =
// = tk_is_symbol_allowed
// =
// =====
bool tk_is_symbol_allowed(char c) {
  const char* allowed = " ,!=<>+-*/^%";

  return is_letter(c) or is_digit(c) or is_bracket(c) or strchr(allowed, c);
}

// =====
// =
// = token_print
// =
// =====
void token_print(const Token* this, OutStream stream) {
  switch (this->type) {
    case TOKEN_IDENT:
      x_sprintf(stream, "%$slice", this->data.ident_text);
      break;
    case TOKEN_NUMBER:
      x_sprintf(stream, "%lf", this->data.number_number);
      break;
    case TOKEN_BRACKET:
      x_sprintf(stream, "%c", this->data.bracket_symbol);
      break;
    case TOKEN_OPERATOR:
      x_sprintf(stream, "<%$slice>", this->data.operator_text);
      break;
    case TOKEN_COMMA:
      x_sprintf(stream, ",");
      break;
    default:
      x_sprintf(stream, "<unknown token type %d>", this->type);
      break;
  }
}

// =====
// =
// = token_type_text
// =
// =====
const char* token_type_text(int token_type) {
  switch (token_type) {
    case TOKEN_IDENT:
      return "Ident";
    case TOKEN_NUMBER:
      return "Number";
    case TOKEN_BRACKET:
      return "Bracket";
    case TOKEN_OPERATOR:
      return "Operator";
    case TOKEN_COMMA:
      return "Comma";
    default:
      panic("Unknown token type %d", token_type);
  }
}

// =====
// ALL THE STATIC GARBAGE
static const char* skip_spaces(const char* string) {
  while (string[0] == ' ') string++;
  return string;
}

static bool check_operator(const char* string, struct TokenResult* result) {
  const char* const operators[] = {
      "mod",                                      // mod
      "..=", "..",                                // Range
      ":=",  "+=", "-=", "*=", "/=", "%=", "^=",  // Procedures
      "==",  "!=", "<=", ">=", "<",  ">",         // Comparsions
      "=",   "+",  "-",  "*",  "/",  "%",  "^",   // Operators
  };
  const int operators_len = LEN(operators);

  bool is_operator = false;
  for (int i = 0; i < operators_len and not is_operator; i++) {
    const char* op = operators[i];
    int op_len = strlen(op);

    if (strncmp(op, string, op_len) is 0) {
      (*result).has_token = true;
      (*result).token.type = TOKEN_OPERATOR;
      (*result).token.data.operator_text = (StrSlice){string, op_len};
      (*result).next_token_pos = string + op_len;
      is_operator = true;
    }
  }

  return is_operator;
}

static struct TokenResult scan_ident(const char* string,
                                     struct TokenResult result) {
  const char* end = string;
  while (is_letter(*end)) end++;

  result.has_token = true;
  result.next_token_pos = end;
  result.token.type = TOKEN_IDENT;
  result.token.data.ident_text = (StrSlice){string, (ptrdiff_t)(end - string)};
  return result;
}

static double read_num_non_scientific(const char* string, int* offset) {
  int len = 0;
  while (string[len] != '\0' and (is_digit(string[len]) or string[len] is '.'))
    len++;

  str_t part = str_owned("%.*s", len, string);
  double number = 42.0;
  int success = sscanf(part.string, "%lf", &number);
  (*offset) = len;
  str_free(part);
  assert_m(success and (*offset) > 0);
  return number;
}

static bool has_double_dot_after_digits(const char* string) {
  int len = strlen(string);
  for (int i = 0; i < len - 2 and is_digit(string[i]); i++)
    if (string[i + 1] == '.' and string[i + 2] == '.') return true;

  return false;
}

static struct TokenResult scan_number(const char* string,
                                      struct TokenResult result) {
  bool is_negative = false;

  if (string[0] is '-') {
    string++;  // Skip '-' symbol
    is_negative = true;
  }

  string = skip_spaces(string);
  bool has_double_dot = has_double_dot_after_digits(string);

  int offset = 0, success = 1;
  double number = 42.0;

  if (not has_double_dot) {
    success = sscanf(string, "%lf%n", &number, &offset);

    if (not success or offset is 0) {
      number = read_num_non_scientific(string, &offset);
      success = 1;
    }
  } else {
    // Scan as integer, since dots are for .. or ..= operators
    long long number_l = 0;
    success = sscanf(string, "%lld%n", &number_l, &offset);
    number = number_l;
  }

  if (not success or offset is 0)
    panic("Failed to parse number at: %s (this SHOULD NOT HAPPEN)\n", string);

  result.has_token = true;
  result.token.data.number_number = number * (is_negative ? -1.0 : 1.0);
  result.next_token_pos = string + offset;
  result.token.type = TOKEN_NUMBER;

  return result;
}

static bool is_letter(char c) {
  return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '_';
}
static bool is_digit(char c) { return c >= '0' and c <= '9'; }

static bool is_bracket(char c) {
  return c is '(' or c is ')' or c is '[' or c is ']' or c is '{' or c is '}';
}

static char next_char_skip_spaces(const char* text) {
  while (text[0] is ' ') text++;
  return text[0];
}
