#include "func_const_ctx.h"

#include "../util/allocator.h"
#include "native_functions.h"

static bool fctx_has_value(const FuncConstCtx* this, StrSlice name);

// Parsing
static bool fctx_is_variable(FuncConstCtx* this, StrSlice var_name);
static bool fctx_is_function(FuncConstCtx* this, StrSlice fun_name);

// Computation
static ExprValueResult fctx_get_variable_val(FuncConstCtx* this, StrSlice);
static ExprValueResult fctx_call_function(FuncConstCtx* this, StrSlice,
                                          vec_ExprValue*);

// Anasysis and compilation
static bool fctx_is_expr_const(FuncConstCtx* this, const Expr* expr);
static int fctx_get_expr_type(FuncConstCtx* this, const Expr* expr);

static ExprVariableInfo fctx_get_variable_info(FuncConstCtx* this,
                                               StrSlice var_name);
static ExprFunctionInfo fctx_get_function_info(FuncConstCtx* this,
                                               StrSlice fun_name);

ExprContext func_const_ctx_context(FuncConstCtx* this) {
  static const ExprContextVtable table = {
      .is_variable = (void*)fctx_is_variable,
      .is_function = (void*)fctx_is_function,
      .get_variable_val = (void*)fctx_get_variable_val,
      .call_function = (void*)fctx_call_function,
      .is_expr_const = (void*)fctx_is_expr_const,
      .get_expr_type = (void*)fctx_get_expr_type,
      .get_variable_info = (void*)fctx_get_variable_info,
      .get_function_info = (void*)fctx_get_function_info,
  };

  return (ExprContext){.data = this, .vtable = &table};
}

static bool fctx_has_value(const FuncConstCtx* this, StrSlice name) {
  for (int i = 0; i < this->used_args->length; i++)
    if (str_slice_eq_ccp(name, this->used_args->data[i].string)) return true;
  return false;
}

// Parsing
static bool fctx_is_variable(FuncConstCtx* this, StrSlice var_name) {
  if (fctx_has_value(this, var_name)) return true;

  return this->parent.vtable->is_variable(this->parent.data, var_name);
}
static bool fctx_is_function(FuncConstCtx* this, StrSlice fun_name) {
  if (fctx_has_value(this, fun_name)) return false;

  return this->parent.vtable->is_function(this->parent.data, fun_name);
}

// Computation
static ExprValueResult fctx_get_variable_val(FuncConstCtx* this,
                                             StrSlice var_name) {
  if (fctx_has_value(this, var_name))
    panic(
        "FuncConstCtx cannot give a value for name-only function arg '%$slice'",
        var_name);

  return this->parent.vtable->get_variable_val(this->parent.data, var_name);
}
static ExprValueResult fctx_call_function(FuncConstCtx* this, StrSlice fun_name,
                                          vec_ExprValue* used_args) {
  if (fctx_has_value(this, fun_name))
    return ExprValueErr(null, str_owned("'%$slice' is not a function, but a "
                                        "parent function argument instead",
                                        fun_name));

  return this->parent.vtable->call_function(this->parent.data, fun_name,
                                            used_args);
}

// Anasysis and compilation
static bool pure_fctx_is_expr_const(FuncConstCtx* this, const Expr* expr);

static bool fctx_is_expr_const(FuncConstCtx* this, const Expr* expr) {
  return pure_fctx_is_expr_const(this, expr);
}

static bool pure_fctx_is_expr_const(FuncConstCtx* this, const Expr* expr) {
  if (expr->type is EXPR_NUMBER) {
    return true;
  } else if (expr->type is EXPR_VARIABLE) {
    if (fctx_has_value(this, str_slice_from_str_t(&expr->variable.name))) {
      return this->are_const;
    } else {
      return this->parent.vtable->is_expr_const(this->parent.data, expr);
    }
  } else if (expr->type is EXPR_FUNCTION) {
    if (fctx_has_value(this, str_slice_from_str_t(&expr->function.name)))
      return false;
    else {
      StrSlice name = str_slice_from_str_t(&expr->function.name);
      bool is_arg_const =
          pure_fctx_is_expr_const(this, expr->function.argument);
      if (calculator_get_native_function(name))
        return is_arg_const;
      else
        return fctx_get_function_info(this, name).is_const and is_arg_const;
    }
  } else if (expr->type is EXPR_VECTOR) {
    for (int i = 0; i < expr->vector.arguments.length; i++)
      if (not pure_fctx_is_expr_const(this, &expr->vector.arguments.data[i]))
        return false;

    return true;
  } else if (expr->type is EXPR_BINARY_OP) {
    return pure_fctx_is_expr_const(this, expr->binary_operator.lhs) and
           pure_fctx_is_expr_const(this, expr->binary_operator.rhs);
  } else {
    panic("Unknown Expr type: %d", expr->type);
  }
}

static int fctx_get_expr_type(FuncConstCtx* this, const Expr* expr) {
  return this->parent.vtable->get_expr_type(this->parent.data, expr);
}

static ExprVariableInfo fctx_get_variable_info(FuncConstCtx* this,
                                               StrSlice var_name) {
  debugln("fctx: someone asks for variable '%$slice'", var_name);
  if (fctx_has_value(this, var_name)) {
    return (ExprVariableInfo){
        .is_const = this->are_const,
        .value_type = VALUE_TYPE_UNKNOWN,
        .value = null,
        .expression = null,  // We dont know arguments values at "compile"-time
        .correct_context = func_const_ctx_context(this),
    };
  } else {
    return this->parent.vtable->get_variable_info(this->parent.data, var_name);
  }
}
static ExprFunctionInfo fctx_get_function_info(FuncConstCtx* this,
                                               StrSlice fun_name) {
  if (fctx_has_value(this, fun_name))
    panic(
        "'%$slice' is not a function, but a parent function argument instead");

  assert_m(this->parent.vtable->get_function_info);
  return this->parent.vtable->get_function_info(this->parent.data, fun_name);
}
