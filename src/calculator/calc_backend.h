#ifndef SRC_CALCULATOR_CALC_BACKEND_H_
#define SRC_CALCULATOR_CALC_BACKEND_H_

#include "../util/better_io.h"
#include "calc_expr.h"
#include "calc_value.h"

ExprValueResult calc_calculate_expr(const char* text, double x, double y);

typedef struct CalcBackend {
  struct CalcBackend* parent;
  vec_CalcExpr expressions;
  vec_CalcValue values;
} CalcBackend;

void calc_backend_free(CalcBackend);
CalcBackend calc_backend_clone(const CalcBackend*);
CalcBackend calc_backend_create();

ExprContext calc_backend_get_context(CalcBackend*);

str_t calc_backend_add_expr(CalcBackend* this, const char* text);

bool calc_backend_is_expr_const(const CalcBackend* this, const Expr* expr);

bool calc_backend_is_func_const(const CalcBackend* this, const char* name);
bool calc_backend_is_var_const(const CalcBackend* this, const char* name);
bool calc_backend_is_func_const_sslice(const CalcBackend* this, StrSlice name);
bool calc_backend_is_var_const_sslice(const CalcBackend* this, StrSlice name);
bool calc_backend_is_func_const_ptr(const CalcBackend* this, CalcExpr* func);
bool calc_backend_is_var_const_ptr(const CalcBackend* this, CalcExpr* var);

CalcValue* calc_backend_get_value(CalcBackend* this, const char* name);
CalcExpr* calc_backend_get_function(CalcBackend* this, const char* name);
CalcExpr* calc_backend_get_variable(CalcBackend* this, const char* name);

CalcValue* calc_backend_get_value_sslice(CalcBackend* this, StrSlice name);
CalcExpr* calc_backend_get_function_sslice(CalcBackend* this, StrSlice name);
CalcExpr* calc_backend_get_variable_sslice(CalcBackend* this, StrSlice name);

ExprContext calc_backend_get_var_context(CalcBackend* this,
                                         const char* var_name);
ExprContext calc_backend_get_var_context_sslice(CalcBackend* this,
                                                StrSlice var_name);

ExprContext calc_backend_get_fun_context(CalcBackend* this,
                                         const char* fun_name);
ExprContext calc_backend_get_fun_context_sslice(CalcBackend* this,
                                                StrSlice fun_name);

CalcExpr* calc_backend_last_expr(CalcBackend* this);
int calc_backend_get_expr_type(const CalcBackend* this, const Expr* expr);

ExprValueResult calc_backend_call_function(CalcBackend* this, StrSlice fun_name,
                                           vec_ExprValue* args_values);

#define VECTOR_H CalcBackend
#include "../util/vector.h"

#endif  // SRC_CALCULATOR_CALC_BACKEND_H_