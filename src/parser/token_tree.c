#include "token_tree.h"

#include <stdio.h>

#include "../util/allocator.h"
#include "../util/common_vecs.h"

#define VECTOR_C TokenTree
#define VECTOR_ITEM_DESTRUCTOR token_tree_free
#include "../util/vector.h"

static TokenTreeResult group_by_brackets(vec_Token tokens);
static TokenTree group_by_functions(TokenTree tree,
                                    bool (*cb_is_function)(void*, StrSlice),
                                    void* cb_data);
static TokenTree group_by_commas(TokenTree tree);

static TokenTree split_by(TokenTree tree, const char* const operators[],
                          int operators_count);
static bool is_operator(const TokenTree* item, const char* const operators[],
                        int operators_count);

static bool is_opening_bracket(char c);
static char flip_bracket(char c);

TokenTreeResult token_tree_parse(const char* text,
                                 bool (*cb_is_function)(void*, StrSlice),
                                 void* cb_data) {
  vec_Token tokens = vec_Token_create();

  // Collect all tokens into vector
  TokenResult next_token = tk_next_token(text);
  while (next_token.has_token) {
    vec_Token_push(&tokens, next_token.token);
    next_token = tk_next_token(next_token.next_token_pos);
  }

  TokenTreeResult result = group_by_brackets(tokens);
  if (result.is_ok) {
    TokenTree tree = result.ok;

    tree = group_by_commas(tree);

    // Each row represents operators of equal priority. Bottom row have more
    // priority that top.
    const char* const operators[] = {
        "=",  ":=",  "+=",  "-=", "*=", "/=", "%=", "^=", "\0", /* - */
        "==", "!=",  "<=",  ">=", "<",  ">",  "\0",             /* - */
        "+",  "-",   "\0",                                      /* - */
        "/",  "%",   "mod", "\0",                               /* - */
        "*",  "\0",                                             /* - */
        "..", "..=", "\0",                                      /* - */
        "^",  "\0",                                             /* - */
    };

    int ops_start = 0;
    for (int i = 0; i < (int)LEN(operators); i++) {
      if (operators[i][0] is '\0') {
        tree = split_by(tree, &operators[ops_start], i - ops_start);
        ops_start = i + 1;
      }
    }

    tree = group_by_functions(tree, cb_is_function, cb_data);
    tree = token_tree_simplify(tree);

    result = (TokenTreeResult){.is_ok = true, .ok = tree};
  }

  debugln("---- FINAL DUMP");
  my_allocator_dump_short();

  return result;
}

void token_tree_free(TokenTree this) {
  if (not this.is_token) vec_TokenTree_free(this.data.tree);
}

void token_tree_print(const TokenTree* tree, OutStream stream) {
  if (tree->is_token) {
    token_print(&tree->data.token, stream);
  } else {
    char bracket = tree->data.tree_bracket ? tree->data.tree_bracket : '<';
    outstream_putc(bracket, stream);
    for (int i = 0; i < tree->data.tree.length; i++) {
      if (i > 0) outstream_putc(' ', stream);
      token_tree_print(&tree->data.tree.data[i], stream);
    }
    outstream_putc(flip_bracket(bracket), stream);
  }
}

#define ERR(textt)                                                       \
  (TokenTreeResult) {                                                    \
    .is_ok = false, .err.text = (textt), .err.text_pos = token.start_pos \
  }

static TokenTree token_tree_bracket(char bracket) {
  return (TokenTree){
      .is_token = false,
      .data.tree = vec_TokenTree_create(),
      .data.tree_bracket = bracket,
  };
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

static void push_vec_tt_to_vec_tt(vec_TokenTree item, vec_TokenTree* into) {
  TokenTree tree = {
      .is_token = false,
      .data.tree_bracket = '{',
      .data.tree = item,
  };
  vec_TokenTree_push(into, tree);
}

#define VAL_TOKEN_COMMA                               \
  (TokenTree) {                                       \
    .is_token = true, .data.token.type = TOKEN_COMMA, \
    .data.token.start_pos = null                      \
  }

static TokenTree group_by_commas(TokenTree tree) {
  if (tree.is_token) return tree;

  vec_TokenTree all_segments = vec_TokenTree_create();
  vec_TokenTree current_tail = vec_TokenTree_create();

  for (int i = 0; i < tree.data.tree.length; i++) {
    TokenTree item = tree.data.tree.data[i];
    item = group_by_commas(item);

    if (tt_token_type(&item) is TOKEN_COMMA) {
      if (all_segments.length > 0)
        vec_TokenTree_push(&all_segments, VAL_TOKEN_COMMA);

      push_vec_tt_to_vec_tt(current_tail, &all_segments);
      current_tail = vec_TokenTree_create();
    } else {
      vec_TokenTree_push(&current_tail, item);
    }
  }
  tree.data.tree.length = 0;  // All elements extracted
  char bracket = tree.data.tree_bracket;
  token_tree_free(tree);

  if (current_tail.length is 0)
    vec_TokenTree_free(current_tail);
  else {
    if (all_segments.length > 0)
      vec_TokenTree_push(&all_segments, VAL_TOKEN_COMMA);
    push_vec_tt_to_vec_tt(current_tail, &all_segments);
  }

  return (TokenTree){.is_token = false,
                     .data.tree_bracket = bracket,
                     .data.tree = all_segments};
}

static void gbb_opening_bracket(Token token, vec_char* stack_brackets,
                                vec_void_ptr* stack,
                                vec_TokenTree* current_pos) {
  vec_char_push(stack_brackets, token.data.bracket_symbol);
  vec_TokenTree_push(current_pos,
                     token_tree_bracket(token.data.bracket_symbol));

  vec_TokenTree* ptr = &current_pos->data[current_pos->length - 1].data.tree;

  vec_void_ptr_push(stack, (void*)ptr);
}

TokenTree token_tree_token(Token token) {
  return (TokenTree){
      .is_token = true,
      .data.token = token,
  };
}

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
      vec_TokenTree_push(current_pos, token_tree_token(token));
    }
  });
  vec_Token_free(tokens);

  vec_void_ptr_free(stack);
  vec_char_free(stack_brackets);

  if (result.is_ok)
    result = (TokenTreeResult){.is_ok = true,
                               .ok = {
                                   .is_token = false,
                                   .data.tree = tree,
                                   .data.tree_bracket = '\0',
                               }};
  else
    vec_TokenTree_free(tree);
  return result;
}

#undef ERR

static TokenTree group_by_functions(TokenTree tree,
                                    bool (*cb_is_function)(void*, StrSlice),
                                    void* cb_data) {
  if (tree.is_token) return tree;

  // We dont need last token
  for (int i = tree.data.tree.length - 2; i >= 0; i--) {
    TokenTree* item_tree = &tree.data.tree.data[i];

    // We seek for an ident that is function name
    if (not item_tree->is_token) continue;
    Token* item = &item_tree->data.token;
    if (item->type is_not TOKEN_IDENT) continue;
    // printf("Token '%.*s' %s a function\n", item->data.ident_text.length,
    // item->data.ident_text.start, cb_is_function(cb_data,
    // item->data.ident_text) ? "is" : "is not");
    if (not cb_is_function(cb_data, item->data.ident_text)) continue;

    // And then group this token and next expression together
    TokenTree group = {
        .is_token = false,
        .data.tree = vec_TokenTree_create(),
        .data.tree_bracket = '\0',
    };
    TokenTree item_owned = vec_TokenTree_extract_order(&tree.data.tree, i);
    TokenTree next_owned = vec_TokenTree_extract_order(&tree.data.tree, i);

    next_owned = group_by_functions(next_owned, cb_is_function, cb_data);

    vec_TokenTree_push(&group.data.tree, item_owned);
    vec_TokenTree_push(&group.data.tree, next_owned);
    vec_TokenTree_insert(&tree.data.tree, group, i);
  }

  return tree;
}

static bool is_operator(const TokenTree* item_ptr,
                        const char* const operators[], int operators_count) {
  bool is_op = false;
  if (item_ptr->is_token and item_ptr->data.token.type is TOKEN_OPERATOR) {
    for (int op = 0; op < operators_count and not is_op; op++) {
      StrSlice op_text = item_ptr->data.token.data.operator_text;
      if (str_slice_eq_ccp(op_text, operators[op])) is_op = true;
    }
  }
  return is_op;
}

static TokenTree split_by(TokenTree tree, const char* const operators[],
                          int operators_count) {
  if (tree.is_token) {
    return tree;
  } else {
    TokenTree result = {
        .is_token = false,
        .data.tree = vec_TokenTree_create(),
        .data.tree_bracket = tree.data.tree_bracket,
    };
    vec_TokenTree tail = vec_TokenTree_create();

    for (int i = 0; i < tree.data.tree.length; i++) {
      TokenTree item =
          split_by(tree.data.tree.data[i], operators, operators_count);

      if (is_operator(&item, operators, operators_count)) {
        TokenTree to_push = {
            .is_token = false,
            .data.tree = tail,
            .data.tree_bracket = '{',
        };
        vec_TokenTree_push(&result.data.tree, to_push);
        tail = vec_TokenTree_create();

        vec_TokenTree_push(&result.data.tree, item);
      } else {
        vec_TokenTree_push(&tail, item);
      }
    }
    tree.data.tree.length = 0;  // Extracted all elements the vector
    token_tree_free(tree);

    TokenTree final = {
        .is_token = false,
        .data.tree = tail,
        .data.tree_bracket = '{',
    };
    vec_TokenTree_push(&result.data.tree, final);

    return result;
  }
}

TokenTree token_tree_simplify(TokenTree tree) {
  if (tree.is_token) return tree;

  TokenTree* only_child = &tree.data.tree.data[0];
  if (tree.data.tree.length is 1 and not only_child->is_token) {
    bool child_is_nonvec_tree =
        not only_child->is_token and only_child->data.tree_bracket != '[';
    bool child_is_vec =
        not only_child->is_token and only_child->data.tree_bracket is '[';
    bool this_is_vec = tree.data.tree_bracket is '[';

    if ((this_is_vec and not child_is_nonvec_tree) or child_is_vec or
        only_child->is_token) {
      (*only_child) = token_tree_simplify(*only_child);
      return tree;  // Do not omit brackets
    }

    TokenTree inner = vec_TokenTree_popget(&tree.data.tree);
    if (not inner.is_token and tree.data.tree_bracket is '[')
      inner.data.tree_bracket = '[';

    TokenTree result = token_tree_simplify(inner);

    token_tree_free(tree);
    return result;
  } else {
    for (int i = tree.data.tree.length - 1; i >= 0; i--) {
      TokenTree* item_ptr = &tree.data.tree.data[i];
      bool is_empty_tree =
          not item_ptr->is_token and item_ptr->data.tree.length is 0;

      if (not is_empty_tree or item_ptr->is_token) {
        (*item_ptr) = token_tree_simplify(*item_ptr);
      } else {
        vec_TokenTree_delete_order(&tree.data.tree, i);
      }
    }
    return tree;
  }
}

TokenTree token_tree_unwrap(TokenTreeResult res) {
  if (not res.is_ok)
    panic("TokenTree unwrap failed: %s (at `...%.10s...`)\n",
          res.err.text.string, res.err.text_pos);

  return res.ok;
}

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

int tt_token_type(TokenTree* item) {
  if (item->is_token)
    return item->data.token.type;
  else
    return -1;
}
