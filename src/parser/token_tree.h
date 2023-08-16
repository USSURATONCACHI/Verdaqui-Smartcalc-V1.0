#ifndef SRC_PARSER_TOKEN_TREE_H_
#define SRC_PARSER_TOKEN_TREE_H_

#include "../util/better_io.h"
#include "../util/better_string.h"
#include "../util/prettify_c.h"
#include "tokenizer.h"

typedef struct TokenTree TokenTree;

// vec_TokenTree header
#define VECTOR_H TokenTree
#include "../util/vector.h"

struct TokenTree {
  bool is_token;
  union {
    Token token;
    struct {
      vec_TokenTree vec;
      char bracket;
    } tree;
  };
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

typedef struct TtContext {
  void* data;
  bool (*is_function)(void*, StrSlice);
} TtContext;

// =====

void token_tree_free(TokenTree this);
TokenTree token_tree_clone(const TokenTree* source);

int token_tree_ttype(const TokenTree* this);
int token_tree_ttype_skip(const TokenTree* this);
Token* token_tree_get_only_token(TokenTree* this);
void token_tree_print(const TokenTree* this, OutStream out);

TokenTreeResult token_tree_parse(const char* text, TtContext ctx);
TokenTree token_tree_simplify(TokenTree tree);
TokenTree token_tree_from_token(Token token);
TokenTree token_tree_from_vec(vec_TokenTree vec, char bracket);

#endif