#ifndef SRC_PARSER_CALCULATOR_H_
#define SRC_PARSER_CALCULATOR_H_

#include "../parser/parser.h"

#define CALC_EXPR_VARIABLE 20  // name
#define CALC_EXPR_FUNCTION 21  // name, args
#define CALC_EXPR_PLOT 22      // just expression
#define CALC_EXPR_ACTION 23

typedef struct CalcExpr {
  Expr expression;
  int type;
  union {
    str_t variable_name;  // unknown ident = smth

    struct {
      str_t name;
      vec_str_t args;
    } function;  // unknown function (ident, ident, ...) = anything

    // plot - nothing
    // action - nothing
  };
} CalcExpr;

void calc_expr_free(CalcExpr this);

#define VECTOR_H CalcExpr
#include "../util/vector.h"  // vec_CalcExpr

typedef struct CalcValue {
  str_t name;
  ExprValue value;
} CalcValue;

void calc_value_free(CalcValue);

#define VECTOR_H CalcValue
#include "../util/vector.h"  // vec_CalcValue

typedef struct CalcBackend {
  struct CalcBackend* parent;
  vec_CalcExpr expressions;
  vec_CalcValue values;
} CalcBackend;

CalcBackend calc_backend_create();
void calc_backend_free(CalcBackend this);

ExprContext calc_backend_get_context(const CalcBackend* this);
str_t calc_backend_add_expr(CalcBackend* this, const char* text);

bool calc_backend_is_const_expr(const CalcBackend* this, const Expr* expr);

bool calc_backend_is_function_constexpr(const CalcBackend* this,
                                        const char* name);

bool calc_backend_is_variable_constexpr(const CalcBackend* this,
                                        const char* name);
CalcValue* calc_backend_get_value(CalcBackend* this, const char* name);
CalcExpr* calc_backend_get_function(CalcBackend* this, const char* name);
CalcExpr* calc_backend_get_variable(CalcBackend* this, const char* name);
CalcExpr* calc_backend_last_expr(CalcBackend* this);

#endif  // SRC_PARSER_CALCULATOR_H_