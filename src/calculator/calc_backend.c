#include "calc_backend.h"
#include "../util/prettify_c.h"

#define VECTOR_C CalcBackend
#define VECTOR_ITEM_DESTRUCTOR calc_backend_free
#define VECTOR_ITEM_CLONE calc_backend_clone
#include "../util/vector.h"

// =====
// =
// BASICS
// =
// =====
void calc_backend_free(CalcBackend this) {
  vec_CalcExpr_free(this.expressions);
  vec_CalcValue_free(this.values);
}

CalcBackend calc_backend_clone(const CalcBackend* this) {
  return (CalcBackend) {
    .parent = this->parent,
    .expressions = vec_CalcExpr_clone(&this->expressions),
    .values = vec_CalcValue_clone(&this->values)
  };
}


// =====
// =
// = calc_backend_create
// =
// =====
#define CONST_NAMES \
  { "pi", "e" }
#define CONST_VALUES \
  { 3.1415926536, 2.7182818284 }

CalcBackend calc_backend_create() {
  const char* const names[] = CONST_NAMES;
  double const values[] = CONST_VALUES;
  CalcBackend result = {
      .parent = null,
      .expressions = vec_CalcExpr_create(),
      .values = vec_CalcValue_with_capacity(LEN(values)),
  };

  assert_m(LEN(names) == LEN(values));
  for (int i = 0; i < (int)LEN(values); i++)
    vec_CalcValue_push(
        &result.values,
        (CalcValue){.name = str_literal(names[i]),
                    .value = {.type = EXPR_VALUE_NUMBER, .number = values[i]}});

  return result;
}


// =====
// =
// =
// =
// =====

str_t calc_backend_add_expr(CalcBackend* this, const char* text) {
  panic("Not yet implemented!");
}

bool calc_backend_is_expr_const(const CalcBackend* this, Expr* expr) {
  panic("Not yet implemented!");
}
bool calc_backend_is_func_const(const CalcBackend* this, CalcExpr* function)  {
  panic("Not yet implemented!");
}
bool calc_backend_is_var_const(const CalcBackend* this, CalcExpr* variable) {
  panic("Not yet implemented!");
}

CalcValue* calc_backend_get_value(CalcBackend* this, const char* name) {
  return calc_backend_get_value_sslice(this, str_slice_from_string(name));
}
CalcExpr* calc_backend_get_function(CalcBackend* this, const char* name) {
  return calc_backend_get_function_sslice(this, str_slice_from_string(name));
}
CalcExpr* calc_backend_get_variable(CalcBackend* this, const char* name) {
  return calc_backend_get_variable_sslice(this, str_slice_from_string(name));
}

CalcValue* calc_backend_get_value_sslice(CalcBackend* this, StrSlice name) {
  for (int i = 0; i < this->values.length; i++)
    if (str_slice_eq_ccp(name, this->values.data[i].name.string))
      return &this->values.data[i];
    
  if (this->parent)
    return calc_backend_get_value_sslice(this->parent, name);
  else
    return null;
}

CalcExpr* calc_backend_get_function_sslice(CalcBackend* this, StrSlice name) {
  for (int i = 0; i < this->expressions.length; i++) {
    CalcExpr* item = &this->expressions.data[i];

    if (item->type is CALC_EXPR_FUNCTION and str_slice_eq_ccp(name, item->function.name.string))
      return item;
  }
    
  if (this->parent)
    return calc_backend_get_function_sslice(this->parent, name);
  else
    return null;
}
CalcExpr* calc_backend_get_variable_sslice(CalcBackend* this, StrSlice name) {
  for (int i = 0; i < this->expressions.length; i++) {
    CalcExpr* item = &this->expressions.data[i];

    if (item->type is CALC_EXPR_VARIABLE and str_slice_eq_ccp(name, item->function.name.string))
      return item;
  }
    
  if (this->parent)
    return calc_backend_get_variable_sslice(this->parent, name);
  else
    return null;
}

CalcExpr* calc_backend_last_expr(CalcBackend* this) {
  if (not this) return null;

  if (this->expressions.length > 0)
    return &this->expressions.data[this->expressions.length - 1];
  else
    return null;
}



// =====
// =
// =
// =
// =====

/*
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
*/
ExprContext calc_backend_get_context(const CalcBackend*) {
  static const ExprContextVtable table = {
    .is_variable = null,
    .is_function = null,

    .get_variable_val = null,
    .call_function = null,

    .is_expr_const = null,
    .get_expr_type = null,
    
    .get_variable_info = null,
    .get_function_info = null,
  };
  panic("Not yet implemented!");
}