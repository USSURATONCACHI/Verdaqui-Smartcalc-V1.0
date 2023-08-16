#ifndef SRC_PARSER_EXPR_H_
#define SRC_PARSER_EXPR_H_

#include "../util/better_io.h"
#include "../util/better_string.h"
#include "expr_value.h"
#include "token_tree.h"

typedef struct Expr Expr;

// ===== vec_Expr
#define VECTOR_H Expr
#include "../util/vector.h"

// ===== vec_vec_Expr
#define VECTOR_H vec_Expr
#include "../util/vector.h"

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

// HELPER TYPES

typedef struct ExprFunctionInfo {
  bool is_const;
  const vec_str_t* args_names;
  const Expr* expression;
  int value_type;
} ExprFunctionInfo;

typedef struct ExprVariableInfo {
  bool is_const;
  const Expr* expression;
  const ExprValue* value;
  int value_type;
} ExprVariableInfo;

typedef struct ExprContextVtable {
  // Parsing
  bool (*is_variable)(void* this, StrSlice var_name);
  bool (*is_function)(void* this, StrSlice fun_name);

  // Computation
  ExprValueResult (*get_variable_val)(void*, StrSlice);
  ExprValueResult (*call_function)(void*, StrSlice, vec_ExprValue*);

  // Anasysis and compilation
  bool (*is_expr_const)(void* this, const Expr* expr);
  int (*get_expr_type)(void* this, const Expr* expr);

  ExprVariableInfo (*get_variable_info)(void* this, StrSlice var_name);
  ExprFunctionInfo (*get_function_info)(void* this, StrSlice fun_name);
} ExprContextVtable;

typedef struct ExprContext {
  void* data;
  const ExprContextVtable* vtable;
} ExprContext;

// FUNCTIONS

// -- Basic functionality
void expr_free(Expr this);
void expr_print(const Expr* this, OutStream out);
Expr expr_clone(const Expr* this);
Expr* expr_move_to_heap(Expr value);
const char* expr_type_text(int type);

// -- Parsing
ExprResult expr_parse_string(const char* text, ExprContext ctx);
ExprResult expr_parse_token_tree(TokenTree tree, ExprContext ctx);
ExprResult expr_parse_tokens(vec_TokenTree tokens, char bracket,
                             ExprContext ctx);

// -- Computation
ExprValueResult expr_calculate(const Expr* this, ExprContext ctx);

/*
Maybe later:
vec_str_t expr_get_used_variables(const Expr* this);
vec_str_t expr_get_used_functions(const Expr* this);

void expr_iter_variables(const Expr* this,
                         void (*callback)(void* cb_data, const str_t* var_name),
                         void* cb_data);
void expr_iter_functions(const Expr* this,
                         void (*callback)(void* cb_data, const str_t* fn_name),
                         void* cb_data);
*/

#endif  // SRC_PARSER_EXPR_H_