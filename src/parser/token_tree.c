// HEADERS
#include "token_tree.h"

#include <stdio.h>

#include "../util/allocator.h"
#include "../util/common_vecs.h"

#define VECTOR_C TokenTree
#define VECTOR_ITEM_DESTRUCTOR token_tree_free
#define VECTOR_ITEM_CLONE token_tree_clone
#include "../util/vector.h"

static bool is_opening_bracket(char c);
static char flip_bracket(char c);

// =====
// =
// = token_tree_free
// =
// =====
void token_tree_free(TokenTree this) {
  if (not this.is_token) vec_TokenTree_free(this.tree.vec);
}

// =====
// =
// = token_tree_clone
// =
// =====
TokenTree token_tree_clone(const TokenTree* source) {
  TokenTree result;
  if (source->is_token) {
    result = *source;  // Tokens can be just copied by value
  } else {
    result = (TokenTree){
        .is_token = false,
        .tree.bracket = source->tree.bracket,
        .tree.vec = vec_TokenTree_clone(&source->tree.vec),
    };
  }
  return result;
}

// =====
// =
// = token_tree_ttype
// =
// =====
int token_tree_ttype(const TokenTree* this) {
  if (this->is_token)
    return this->token.type;
  else
    return -1;
}

// =====
// =
// = token_tree_ttype_skip
// =
// =====
int token_tree_ttype_skip(const TokenTree* this) {
  if (this->is_token)
    return this->token.type;
  else if (this->tree.vec.length is 1)
    return token_tree_ttype_skip(&this->tree.vec.data[0]);
  else
    return -1;
}

// =====
// =
// = token_tree_get_only_token
// =
// =====
Token* token_tree_get_only_token(TokenTree* this) {
  if (this->is_token)
    return &this->token;
  else if (this->tree.vec.length is 1)
    return token_tree_get_only_token(&this->tree.vec.data[0]);
  else
    return null;
}

// =====
// =
// = token_tree_print
// =
// =====
void token_tree_print(const TokenTree* tree, OutStream stream) {
  if (tree->is_token) {
    token_print(&tree->token, stream);
  } else {
    char bracket = tree->tree.bracket ? tree->tree.bracket : '<';
    outstream_putc(bracket, stream);

    for (int i = 0; i < tree->tree.vec.length; i++) {
      if (i > 0) outstream_putc(' ', stream);
      token_tree_print(&tree->tree.vec.data[i], stream);
    }

    outstream_putc(flip_bracket(bracket), stream);
  }
}

// =====
// =
// = token_tree_from_token
// =
// =====
TokenTree token_tree_from_token(Token token) {
  return (TokenTree){.is_token = true, .token = token};
}

// =====
// =
// = token_tree_from_vec
// =
// =====
TokenTree token_tree_from_vec(vec_TokenTree vec, char bracket) {
  return (TokenTree){
      .is_token = false, .tree.vec = vec, .tree.bracket = bracket};
}

// =====
// =
// = token_tree_simplify
// =
// =====
static TokenTree simplify_wrapper_tree(TokenTree tree);
static TokenTree simplify_vector_tree(TokenTree tree);

TokenTree token_tree_simplify(TokenTree tree) {
  if (tree.is_token) return tree;

  TokenTree* only_child = &tree.tree.vec.data[0];

  if (tree.tree.vec.length is 1 and not only_child->is_token)
    return simplify_wrapper_tree(tree);
  else
    return simplify_vector_tree(tree);
}

static TokenTree simplify_wrapper_tree(TokenTree tree) {
  assert_m(not tree.is_token and tree.tree.vec.length is 1);

  TokenTree* only_child = &tree.tree.vec.data[0];

  bool child_is_nonvec_tree =
      not only_child->is_token and only_child->tree.bracket != '[';
  bool child_is_vec =
      not only_child->is_token and only_child->tree.bracket is '[';
  bool this_is_vec = tree.tree.bracket is '[';

  bool should_not_omit = (this_is_vec and not child_is_nonvec_tree) or
                         child_is_vec or only_child->is_token;

  TokenTree result;
  if (should_not_omit) {
    (*only_child) = token_tree_simplify(*only_child);
    result = tree;  // Do not omit brackets
  } else {
    TokenTree inner = vec_TokenTree_popget(&tree.tree.vec);
    if (not inner.is_token and
        tree.tree.bracket is '[')  // [(...)] should become just [...]
      inner.tree.bracket = '[';

    result = token_tree_simplify(inner);

    token_tree_free(tree);
  }

  return result;
}

static TokenTree simplify_vector_tree(TokenTree tree) {
  for (int i = tree.tree.vec.length - 1; i >= 0; i--) {
    TokenTree* item_ptr = &tree.tree.vec.data[i];

    bool is_empty_tree =
        not item_ptr->is_token and item_ptr->tree.vec.length is 0;

    if (not is_empty_tree) {
      (*item_ptr) = token_tree_simplify(*item_ptr);
    } else {
      vec_TokenTree_delete_order(&tree.tree.vec, i);  // Omit empty token tree.
    }
  }
  return tree;
}

// =====
// =
// = token_tree_parse
// =
// =====

// Each row represents operators of equal priority. Bottom row have more
// priority that top.
#define OPERATORS                                                \
  {                                                              \
    "=", ":=", "+=", "-=", "*=", "/=", "%=", "^=", "\0", /* - */ \
        "==", "!=", "<=", ">=", "<", ">", "\0",          /* - */ \
        "+", "-", "\0",                                  /* - */ \
        "/", "%", "mod", "\0",                           /* - */ \
        "*", "\0",                                       /* - */ \
        "..", "..=", "\0",                               /* - */ \
        "^", "\0",                                       /* - */ \
  }

typedef struct OpsSlice {
  const char* const* operators;
  int length;
} OpsSlice;

static TokenTreeResult group_by_brackets(vec_Token tokens);
static TokenTree group_by_commas(TokenTree tree);
static TokenTree split_by(TokenTree tree, OpsSlice operators);
static TokenTree group_by_functions(TokenTree tree, TtContext ctx);

TokenTreeResult token_tree_parse(const char* text, TtContext ctx) {
  vec_Token tokens = vec_Token_create();

  // Collect all tokens into vector
  TokenResult next_token = tk_next_token(text);
  while (next_token.has_token) {
    vec_Token_push(&tokens, next_token.token);
    next_token = tk_next_token(next_token.next_token_pos);
  }

  TokenTreeResult result = group_by_brackets(tokens);
  if (result.is_ok) {
    TokenTree tree = group_by_commas(result.ok);

    // Split by all operators
    static const char* const operators[] = OPERATORS;

    int ops_start = 0;
    for (int i = 0; i < (int)LEN(operators); i++) {
      if (operators[i][0] is '\0') {
        tree = split_by(tree, (OpsSlice){.operators = &operators[ops_start],
                                         .length = i - ops_start});
        ops_start = i + 1;
      }
    }

    // TODO: GROUP BY IMPLICIT MUL
    tree = group_by_functions(tree, ctx);
    tree = token_tree_simplify(tree);

    result = (TokenTreeResult){.is_ok = true, .ok = tree};
  } else {
    // nothing
  }

  return result;
}

// Group by brackets
#define ERR(textt)                                                       \
  (TokenTreeResult) {                                                    \
    .is_ok = false, .err.text = (textt), .err.text_pos = token.start_pos \
  }

static void gbb_closing_bracket(Token token, vec_char* stack_brackets,
                                vec_void_ptr* stack, TokenTreeResult* result);

static void gbb_opening_bracket(Token token, vec_char* stack_brackets,
                                vec_void_ptr* stack,
                                vec_TokenTree* current_pos);

static TokenTreeResult group_by_brackets(vec_Token tokens) {
  vec_TokenTree tree = vec_TokenTree_create();
  vec_void_ptr stack = vec_void_ptr_create();
  vec_char stack_brackets = vec_char_create();

  vec_void_ptr_push(&stack, &tree);
  TokenTreeResult result = {.is_ok = true};

  foreach_extract(Token token, tokens, result.is_ok, {
    vec_TokenTree* current_pos = (vec_TokenTree*)stack.data[stack.length - 1];

    if (token.type is TOKEN_BRACKET) {
      if (is_opening_bracket(token.data.bracket_symbol))
        gbb_opening_bracket(token, &stack_brackets, &stack, current_pos);
      else
        gbb_closing_bracket(token, &stack_brackets, &stack, &result);
    } else {
      vec_TokenTree_push(current_pos, token_tree_from_token(token));
    }
  });
  vec_Token_free(tokens);

  vec_void_ptr_free(stack);
  vec_char_free(stack_brackets);

  if (result.is_ok)
    result = (TokenTreeResult){.is_ok = true,
                               .ok = {
                                   .is_token = false,
                                   .tree.vec = tree,
                                   .tree.bracket = '\0',
                               }};
  else
    vec_TokenTree_free(tree);
  return result;
}

static void gbb_closing_bracket(Token token, vec_char* stack_brackets,
                                vec_void_ptr* stack, TokenTreeResult* result) {
  // CLOSING BRACKET
  char matching_bracket = flip_bracket(token.data.bracket_symbol);

  if (stack_brackets->length is 0)
    (*result) = ERR(str_literal("Unexpected closing bracket"));

  else if (vec_char_popget(stack_brackets) == matching_bracket)
    vec_void_ptr_popfree(stack);

  else
    (*result) = ERR(str_literal("Closing bracket does not match"));
}

static TokenTree token_tree_bracket(char bracket);

static void gbb_opening_bracket(Token token, vec_char* stack_brackets,
                                vec_void_ptr* stack,
                                vec_TokenTree* current_pos) {
  vec_char_push(stack_brackets, token.data.bracket_symbol);
  vec_TokenTree_push(current_pos,
                     token_tree_bracket(token.data.bracket_symbol));

  vec_TokenTree* ptr = &current_pos->data[current_pos->length - 1].tree.vec;

  vec_void_ptr_push(stack, (void*)ptr);
}

static TokenTree token_tree_bracket(char bracket) {
  return (TokenTree){
      .is_token = false,
      .tree.vec = vec_TokenTree_create(),
      .tree.bracket = bracket,
  };
}

// SPLIT BY

static bool is_operator(const TokenTree* item, OpsSlice operators);

static TokenTree split_by(TokenTree tree, OpsSlice operators) {
  if (tree.is_token) {
    return tree;
  } else {
    TokenTree result = {
        .is_token = false,
        .tree.vec = vec_TokenTree_create(),
        .tree.bracket = tree.tree.bracket,
    };
    vec_TokenTree tail = vec_TokenTree_create();

    for (int i = 0; i < tree.tree.vec.length; i++) {
      TokenTree item = split_by(tree.tree.vec.data[i], operators);

      if (is_operator(&item, operators)) {
        TokenTree to_push = {
            .is_token = false,
            .tree.vec = tail,
            .tree.bracket = '{',
        };
        vec_TokenTree_push(&result.tree.vec, to_push);
        tail = vec_TokenTree_create();

        vec_TokenTree_push(&result.tree.vec, item);
      } else {
        vec_TokenTree_push(&tail, item);
      }
    }
    tree.tree.vec.length = 0;  // Extracted all elements the vector
    token_tree_free(tree);

    TokenTree final = {
        .is_token = false,
        .tree.vec = tail,
        .tree.bracket = '{',
    };
    vec_TokenTree_push(&result.tree.vec, final);

    return result;
  }
}

static bool is_operator(const TokenTree* item_ptr, OpsSlice operators) {
  bool is_op = false;
  if (item_ptr->is_token and item_ptr->token.type is TOKEN_OPERATOR) {
    for (int op = 0; op < operators.length and not is_op; op++) {
      StrSlice op_text = item_ptr->token.data.operator_text;
      if (str_slice_eq_ccp(op_text, operators.operators[op])) is_op = true;
    }
  }
  return is_op;
}

// GROUP BY COMMAS

#define VAL_TOKEN_COMMA                                                  \
  (TokenTree) {                                                          \
    .is_token = true, .token.type = TOKEN_COMMA, .token.start_pos = null \
  }

static TokenTree group_by_commas(TokenTree tree) {
  if (tree.is_token) return tree;

  vec_TokenTree all_segments = vec_TokenTree_create();
  vec_TokenTree current_tail = vec_TokenTree_create();

  for (int i = 0; i < tree.tree.vec.length; i++) {
    TokenTree item = tree.tree.vec.data[i];
    item = group_by_commas(item);

    if (token_tree_ttype(&item) is TOKEN_COMMA) {
      if (all_segments.length > 0)
        vec_TokenTree_push(&all_segments, VAL_TOKEN_COMMA);

      vec_TokenTree_push(&all_segments, token_tree_from_vec(current_tail, '{'));
      current_tail = vec_TokenTree_create();
    } else {
      vec_TokenTree_push(&current_tail, item);
    }
  }
  tree.tree.vec.length = 0;  // All elements extracted
  char bracket = tree.tree.bracket;
  token_tree_free(tree);

  if (current_tail.length is 0)
    vec_TokenTree_free(current_tail);
  else {
    if (all_segments.length > 0)
      vec_TokenTree_push(&all_segments, VAL_TOKEN_COMMA);

    vec_TokenTree_push(&all_segments, token_tree_from_vec(current_tail, '{'));
  }

  return (TokenTree){
      .is_token = false, .tree.bracket = bracket, .tree.vec = all_segments};
}

// GROUP BY FUNCTIONS

static TokenTree group_by_functions(TokenTree tree, TtContext ctx) {
  if (tree.is_token) return tree;

  // We dont need last token
  for (int i = tree.tree.vec.length - 2; i >= 0; i--) {
    TokenTree* item_tree = &tree.tree.vec.data[i];

    if (token_tree_ttype(item_tree) is_not TOKEN_IDENT) continue;
    if (not ctx.is_function(ctx.data, item_tree->token.data.ident_text))
      continue;

    // And then group this token and next expression together
    TokenTree group = {
        .is_token = false,
        .tree.vec = vec_TokenTree_create(),
        .tree.bracket = '{',
    };
    TokenTree item_owned = vec_TokenTree_extract_order(&tree.tree.vec, i);
    TokenTree next_owned = vec_TokenTree_extract_order(&tree.tree.vec, i);

    next_owned = group_by_functions(next_owned, ctx);

    vec_TokenTree_push(&group.tree.vec, item_owned);
    vec_TokenTree_push(&group.tree.vec, next_owned);
    vec_TokenTree_insert(&tree.tree.vec, group, i);
  }

  return tree;
}

// OTHER

static bool is_opening_bracket(char c) {
  return c is '(' or c is '{' or c is '[';
}

static char flip_bracket(char c) {
  switch (c) {
    case '(':
      return ')';
    case '[':
      return ']';
    case '{':
      return '}';
    case '<':
      return '>';

    case ')':
      return '(';
    case ']':
      return '[';
    case '}':
      return '{';
    case '>':
      return '<';
    default:
      panic("Unknown bracket: '%c'", c);
  }
}
