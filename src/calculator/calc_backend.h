#ifndef SRC_CALCULATOR_CALC_BACKEND_H_
#define SRC_CALCULATOR_CALC_BACKEND_H_

#include "calc_expr.h"
#include "calc_value.h"
#include "../util/better_io.h"

typedef struct CalcBackend {
  struct CalcBackend* parent;
  vec_CalcExpr expressions;
  vec_CalcValue values;
} CalcBackend;

void calc_backend_free(CalcBackend);
CalcBackend calc_backend_clone(const CalcBackend*);
CalcBackend calc_backend_create();


ExprContext calc_backend_get_context(const CalcBackend*);


str_t calc_backend_add_expr(CalcBackend* this, const char* text);

bool calc_backend_is_expr_const(const CalcBackend* this, Expr* expr);
bool calc_backend_is_func_const(const CalcBackend* this, CalcExpr* function);
bool calc_backend_is_var_const(const CalcBackend* this, CalcExpr* variable);

CalcValue* calc_backend_get_value(CalcBackend* this, const char* name);
CalcExpr* calc_backend_get_function(CalcBackend* this, const char* name);
CalcExpr* calc_backend_get_variable(CalcBackend* this, const char* name);

CalcValue* calc_backend_get_value_sslice(CalcBackend* this, StrSlice name);
CalcExpr* calc_backend_get_function_sslice(CalcBackend* this, StrSlice name);
CalcExpr* calc_backend_get_variable_sslice(CalcBackend* this, StrSlice name);

CalcExpr* calc_backend_last_expr(CalcBackend* this);

#define VECTOR_H CalcBackend
#include "../util/vector.h"

#endif // SRC_CALCULATOR_CALC_BACKEND_H_