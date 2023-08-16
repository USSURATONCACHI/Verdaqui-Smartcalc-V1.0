#ifndef SRC_PARSER_TOKENIZER_H_
#define SRC_PARSER_TOKENIZER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "../util/better_io.h"
#include "../util/better_string.h"

#define TOKEN_IDENT 1
#define TOKEN_NUMBER 2
#define TOKEN_BRACKET 3
#define TOKEN_OPERATOR 4
#define TOKEN_COMMA 5

typedef struct Token {
  int type;
  const char* start_pos;

  union {
    // null - no data
    StrSlice ident_text;
    double number_number;
    char bracket_symbol;
    StrSlice operator_text;
    // comma - no data
  } data;
} Token;

// vec_Token header
#define VECTOR_H Token
#include "../util/vector.h"

typedef struct TokenResult {
  bool has_token;
  Token token;
  const char* next_token_pos;
} TokenResult;

struct TokenResult tk_next_token(const char* string);
bool tk_is_symbol_allowed(char c);

void token_print(const Token* this, OutStream stream);
const char* token_type_text(int token_type);

#endif  // SRC_PARSER_TOKENIZER_H_