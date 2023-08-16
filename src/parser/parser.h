#ifndef SRC_PARSER_PARSER_H_
#define SRC_PARSER_PARSER_H_

#include <stdbool.h>
#include <stdio.h>

#include "../util/better_io.h"
#include "../util/better_string.h"
#include "../util/common_vecs.h"
#include "token_tree.h"
#include "tokenizer.h"

typedef struct Expr Expr;

typedef struct ExprValue ExprValue;

// ===== vec_Expr
#define VECTOR_H Expr
#include "../util/vector.h"

// ===== vec_vec_Expr
#define VECTOR_H vec_Expr
#include "../util/vector.h"

// ===== vec_ExprValue
#define VECTOR_H ExprValue
#include "../util/vector.h"

#define EXPR_VALUE_NUMBER 0
#define EXPR_VALUE_VEC 1
#define EXPR_VALUE_NONE 3
struct ExprValue {
  int type;
  union {
    double number;
    vec_ExprValue vec;
  };
};

void expr_value_free(ExprValue this);
ExprValue expr_value_clone(const ExprValue* source);
void expr_value_print(const ExprValue* this, OutStream stream);

char* expr_value_str_print(const ExprValue* this, char* old_string);
const char* expr_value_type_text(int type);

typedef struct ExprValueResult {
  bool is_ok;

  union {
    ExprValue ok;
    struct {
      str_t err_text;
      const char* err_pos;
    };
  };
} ExprValueResult;

#define EXPR_NUMBER 1
#define EXPR_VARIABLE 2
#define EXPR_FUNCTION 3
#define EXPR_VECTOR 5
#define EXPR_BINARY_OP 7

typedef struct ExprNumber {
  double value;
} ExprNumber;

typedef struct ExprVariable {
  str_t name;
} ExprVariable;

typedef struct ExprFunction {
  str_t name;
  Expr* argument;
} ExprFunction;

typedef struct ExprVector {
  vec_Expr arguments;
} ExprVector;

typedef struct ExprBinaryOp {
  str_t name;
  Expr* lhs;
  Expr* rhs;
} ExprBinaryOp;

struct Expr {
  int type;
  union {
    ExprNumber number;
    ExprVariable variable;
    ExprFunction function;
    ExprVector vector;
    ExprBinaryOp binary_operator;
  };
};
void expr_free(Expr this);

typedef struct ExprResult {
  bool is_ok;

  union {
    Expr ok;

    struct {
      str_t err_text;
      const char* err_pos;
    };
  };
} ExprResult;

typedef struct VecExprResult {
  bool is_ok;

  union {
    vec_Expr ok;
    struct {
      str_t err_text;
      const char* err_pos;
    };
  };
} VecExprResult;

VecExprResult ExprResult_into_VecExprResult(ExprResult from);

typedef struct ExprContext {
  bool (*is_function)(void*, StrSlice);

  ExprValueResult (*get_variable)(void*, StrSlice);

  ExprValueResult (*call_function)(void*, StrSlice, vec_ExprValue*);

  void* context_data;
} ExprContext;

void expr_print(const Expr* this, OutStream out);

ExprValueResult expr_calculate_val(const Expr* this, ExprContext ctx);
ExprResult expr_parse_tokens(vec_TokenTree tokens, char bracket,
                             ExprContext ctx);
ExprResult expr_parse_token_tree(TokenTree tokens, ExprContext ctx);
ExprResult expr_parse_string(const char* text, ExprContext ctx);

vec_str_t expr_get_used_variables(const Expr* this);
vec_str_t expr_get_used_functions(const Expr* this);

void expr_iter_variables(const Expr* this,
                         void (*callback)(void* cb_data, const str_t* var_name),
                         void* cb_data);
void expr_iter_functions(const Expr* this,
                         void (*callback)(void* cb_data, const str_t* fn_name),
                         void* cb_data);

char* expr_str_print(const Expr* this, char* out);

#endif  // SRC_PARSER_PARSER_H_