#ifndef SRC_PARSER_TOKEN_TREE_H_
#define SRC_PARSER_TOKEN_TREE_H_

#include <stdio.h>

#include "../util/better_string.h"
#include "../util/prettify_c.h"
#include "tokenizer.h"

typedef struct TokenTree TokenTree;
void token_tree_free(TokenTree this);

// vec_TokenTree header
#define VECTOR_H TokenTree
#include "../util/vector.h"

struct TokenTree {
  bool is_token;
  union {
    Token token;
    struct {
      vec_TokenTree tree;
      char tree_bracket;
    };
  } data;
};

typedef struct TokenTreeError {
  str_t text;
  const char* text_pos;
} TokenTreeError;

typedef struct TokenTreeResult {
  bool is_ok;
  union {
    TokenTree ok;
    TokenTreeError err;
  };
} TokenTreeResult;

TokenTree token_tree_unwrap(TokenTreeResult res);

TokenTree token_tree_token(Token token);

TokenTreeResult token_tree_parse(const char* text,
                                 bool (*cb_is_function)(void*, StrSlice),
                                 void* cb_data);
TokenTree token_tree_simplify(TokenTree tree);

void token_tree_print(const TokenTree* tree, OutStream stream);

int tt_token_type(TokenTree* item);

#endif