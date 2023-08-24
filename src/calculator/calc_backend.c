#include "calc_backend.h"

#include "../util/allocator.h"
#include "../util/prettify_c.h"
#include "func_const_ctx.h"
#include "native_functions.h"

#define VECTOR_C CalcBackend
#define VECTOR_ITEM_DESTRUCTOR calc_backend_free
#define VECTOR_ITEM_CLONE calc_backend_clone
#include "../util/vector.h"

typedef struct XyValuesContext {
  double x;
  double y;
  ExprContext parent;
} XyValuesContext;

ExprValueResult xy_get_variable_val(XyValuesContext* this, StrSlice name);
ExprValueResult xy_call_function(XyValuesContext* this, StrSlice name,
                                 vec_ExprValue* args);
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

ExprValueResult calc_calculate_expr(const char* text, double x, double y) {
  static const ExprContextVtable XY_CTX_VTABLE = {
      .get_expr_type = null,
      .get_variable_info = null,
      .get_function_info = null,
      .is_variable = null,
      .is_function = null,

      .get_variable_val =
          (ExprValueResult(*)(void*, StrSlice))xy_get_variable_val,
      .call_function =
          (ExprValueResult(*)(void*, StrSlice, vec_ExprValue*))xy_call_function,
  };

  CalcBackend backend = calc_backend_create();
  ExprContext ctx = calc_backend_get_context(&backend);

  ExprResult expr = expr_parse_string(text, ctx);
  ExprValueResult result;
  if (not expr.is_ok) {
    result = ExprValueErr(expr.err_pos, expr.err_text);
  } else {
    XyValuesContext xy_ctx = {.x = x, .y = y, .parent = ctx};
    ExprContext local_ctx = {.data = &xy_ctx, .vtable = &XY_CTX_VTABLE};

    result = expr_calculate(&expr.ok, local_ctx);
    expr_free(expr.ok);
  }

  calc_backend_free(backend);
  return result;
}

ExprValueResult xy_get_variable_val(XyValuesContext* this, StrSlice name) {
  if (str_slice_eq_ccp(name, "x")) {
    ExprValue val = {.type = EXPR_VALUE_NUMBER, .number = this->x};
    return ExprValueOk(val);
  } else if (str_slice_eq_ccp(name, "y")) {
    ExprValue val = {.type = EXPR_VALUE_NUMBER, .number = this->y};
    return ExprValueOk(val);
  } else
    return this->parent.vtable->get_variable_val(this->parent.data, name);
}

ExprValueResult xy_call_function(XyValuesContext* this, StrSlice name,
                                 vec_ExprValue* args) {
  return this->parent.vtable->call_function(this->parent.data, name, args);
}

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
  return (CalcBackend){.parent = this->parent,
                       .expressions = vec_CalcExpr_clone(&this->expressions),
                       .values = vec_CalcValue_clone(&this->values)};
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

bool calc_backend_is_expr_const(const CalcBackend* this, const Expr* expr) {
  bool result = false;

  if (expr->type is EXPR_VARIABLE) {
    result = calc_backend_is_var_const(this, expr->variable.name.string);

  } else if (expr->type is EXPR_NUMBER) {
    result = true;

  } else if (expr->type is EXPR_VECTOR) {
    result = true;
    for (int i = 0; i < expr->vector.arguments.length; i++)
      result = result and calc_backend_is_expr_const(
                              this, &expr->vector.arguments.data[i]);

  } else if (expr->type is EXPR_BINARY_OP) {
    result = calc_backend_is_expr_const(this, expr->binary_operator.lhs) and
             calc_backend_is_expr_const(this, expr->binary_operator.rhs);

  } else if (expr->type is EXPR_FUNCTION) {
    result = calc_backend_is_func_const(this, expr->function.name.string) and
             calc_backend_is_expr_const(this, expr->function.argument);

  } else {
    panic("Unknown expr type");
  }

  return result;
}

bool calc_backend_is_func_const(const CalcBackend* this, const char* name) {
  StrSlice slice = str_slice_from_string(name);
  return calc_backend_is_func_const_sslice(this, slice);
}

bool calc_backend_is_var_const(const CalcBackend* this, const char* name) {
  StrSlice slice = str_slice_from_string(name);
  return calc_backend_is_var_const_sslice(this, slice);
}

bool calc_backend_is_func_const_sslice(const CalcBackend* this, StrSlice name) {
  if (calculator_get_native_function(name)) return true;

  CalcExpr* expr = calc_backend_get_function_sslice((CalcBackend*)this, name);
  if (expr) {
    ExprContext ctx =
        calc_backend_get_fun_context_sslice((CalcBackend*)this, name);
    assert_m(ctx.data);
    FuncConstCtx func_ctx = {
        .parent = ctx,
        .used_args = &expr->function.args,
        .are_const = true,
    };

    ExprContext total_ctx = func_const_ctx_context(&func_ctx);

    return total_ctx.vtable->is_expr_const(total_ctx.data, &expr->expression);
  } else {
    return false;
  }
}

bool calc_backend_is_func_const_ptr(const CalcBackend* this, CalcExpr* expr) {
  assert_m(expr->type is CALC_EXPR_FUNCTION);
  if (expr) {
    ExprContext ctx = calc_backend_get_context((CalcBackend*)this);
    assert_m(ctx.data);
    FuncConstCtx func_ctx = {
        .parent = ctx,
        .used_args = &expr->function.args,
        .are_const = true,
    };

    ExprContext total_ctx = func_const_ctx_context(&func_ctx);

    // debugln("Checking if function is const...");
    // debug_push();
    bool res =
        total_ctx.vtable->is_expr_const(total_ctx.data, &expr->expression);
    // debug_pop();
    // debugln("Checked: %b", res);

    return res;
  } else {
    return false;
  }
}

bool calc_backend_is_var_const_sslice(const CalcBackend* this, StrSlice name) {
  if (calc_backend_get_value_sslice((CalcBackend*)this, name)) {
    return true;
  } else {
    CalcExpr* expr = calc_backend_get_variable_sslice((CalcBackend*)this, name);
    if (expr) {
      ExprContext ctx =
          calc_backend_get_var_context_sslice((CalcBackend*)this, name);
      assert_m(ctx.data);
      return ctx.vtable->is_expr_const(ctx.data, &expr->expression);
    } else {
      return false;
    }
  }
}

bool calc_backend_is_var_const_ptr(const CalcBackend* this, CalcExpr* expr) {
  assert_m(expr->type is CALC_EXPR_VARIABLE);
  if (expr) {
    ExprContext ctx = calc_backend_get_context((CalcBackend*)this);
    assert_m(ctx.data);
    return ctx.vtable->is_expr_const(ctx.data, &expr->expression);
  } else {
    return false;
  }
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
  for (int i = 0; i < this->values.length; i++)
    if (str_slice_eq_ccp(name, this->values.data[i].name.string)) return null;

  for (int i = 0; i < this->expressions.length; i++) {
    CalcExpr* item = &this->expressions.data[i];

    if (item->type is CALC_EXPR_FUNCTION and
        str_slice_eq_ccp(name, item->function.name.string))
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

    if (item->type is CALC_EXPR_VARIABLE and
        str_slice_eq_ccp(name, item->variable_name.string))
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

// CONTEXTS

ExprContext calc_backend_get_var_context(CalcBackend* this,
                                         const char* var_name) {
  StrSlice slice = str_slice_from_string(var_name);
  return calc_backend_get_var_context_sslice(this, slice);
}

ExprContext calc_backend_get_var_context_sslice(CalcBackend* this,
                                                StrSlice var_name) {
  for (int i = 0; i < this->values.length; i++)
    if (str_slice_eq_ccp(var_name, this->values.data[i].name.string))
      return calc_backend_get_context(this);

  for (int i = 0; i < this->expressions.length; i++) {
    CalcExpr* item = &this->expressions.data[i];
    if (item->type is CALC_EXPR_VARIABLE and
        str_slice_eq_ccp(var_name, item->variable_name.string))
      return calc_backend_get_context(this);
  }

  if (this->parent)
    return calc_backend_get_var_context_sslice(this->parent, var_name);
  else
    return calc_backend_get_context(null);
}

ExprContext calc_backend_get_fun_context(CalcBackend* this,
                                         const char* fun_name) {
  StrSlice slice = str_slice_from_string(fun_name);
  return calc_backend_get_fun_context_sslice(this, slice);
}
ExprContext calc_backend_get_fun_context_sslice(CalcBackend* this,
                                                StrSlice fun_name) {
  for (int i = 0; i < this->expressions.length; i++) {
    CalcExpr* item = &this->expressions.data[i];
    if (item->type is CALC_EXPR_FUNCTION and
        str_slice_eq_ccp(fun_name, item->function.name.string))
      return calc_backend_get_context(this);
  }

  if (this->parent)
    return calc_backend_get_fun_context_sslice(this->parent, fun_name);
  else
    return calc_backend_get_context(null);
}

// =====
// =
// = CALL_FUNCTION
// =
// =====

ExprValueResult calc_backend_call_function(CalcBackend* this, StrSlice fun_name,
                                           vec_ExprValue* args_values) {
  // 1. NATIVE
  const NativeFnPtr native_fn = calculator_get_native_function(fun_name);
  if (native_fn) return native_fn(vec_ExprValue_clone(args_values));

  CalcExpr* fn_calc_expr = calc_backend_get_function_sslice(this, fun_name);
  ExprValueResult result;

  if (fn_calc_expr) {
    // 2. Build local backend for function
    CalcBackend nested_backend = calc_backend_create();
    nested_backend.parent = this;

    for (int i = 0; i < fn_calc_expr->function.args.length; i++) {
      CalcValue arg = {
          .name = str_borrow(&fn_calc_expr->function.args.data[i]),
          .value = i < args_values->length
                       ? expr_value_clone(&args_values->data[i])
                       : (ExprValue){.type = EXPR_VALUE_NONE},
      };
      vec_CalcValue_push(&nested_backend.values, arg);
    }

    result = expr_calculate(&fn_calc_expr->expression,
                            calc_backend_get_context(&nested_backend));

    calc_backend_free(nested_backend);
  } else {
    result = ExprValueErr(
        null,
        str_owned("Function '%$slice' is not found (this shoudn't happen btw)",
                  fun_name));
  }

  return result;
}

// =====
// =
// = GET TYPE
// =
// =====
int calc_backend_get_expr_type(const CalcBackend* this, const Expr* expr) {
  int result = VALUE_TYPE_UNKNOWN;

  if (calc_backend_is_expr_const(this, expr)) {
    ExprContext ctx = calc_backend_get_context((CalcBackend*)this);
    ExprValueResult res = expr_calculate(expr, ctx);
    if (res.is_ok) {
      result = res.ok.type;
      expr_value_free(res.ok);
    } else {
      str_free(res.err_text);
    }
  } else {
    debugln("Not yet implemented! Expression: '%$expr'", *expr);
  }

  return result;
}
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

static bool cb_is_variable(CalcBackend* this, StrSlice var_name);
static bool cb_is_function(CalcBackend* this, StrSlice fun_name);

static ExprValueResult cb_get_variable_val(CalcBackend*, StrSlice);
// calc_backend_call_function

static int cb_get_expr_type(CalcBackend* this, const Expr* expr);

ExprVariableInfo cb_get_variable_info(CalcBackend* this, StrSlice var_name);
ExprFunctionInfo cb_get_function_info(CalcBackend* this, StrSlice fun_name);

static bool cb_is_variable(CalcBackend* this, StrSlice var_name) {
  if (calculator_get_native_function(var_name)) return false;

  return calc_backend_get_value_sslice(this, var_name) or
         calc_backend_get_variable_sslice(this, var_name);
}
static bool cb_is_function(CalcBackend* this, StrSlice fun_name) {
  if (calculator_get_native_function(fun_name)) return true;

  return calc_backend_get_function_sslice(this, fun_name);
}

static ExprValueResult cb_get_variable_val(CalcBackend* this,
                                           StrSlice var_name) {
  ExprValueResult result;
  CalcValue* val = calc_backend_get_value_sslice(this, var_name);

  if (val) {
    result = ExprValueOk(expr_value_clone(&val->value));
  } else {
    CalcExpr* expr = calc_backend_get_variable_sslice(this, var_name);

    if (expr) {
      // calculate
      if (calc_backend_is_var_const_sslice(this, var_name)) {
        ExprContext ctx = calc_backend_get_var_context_sslice(this, var_name);
        assert_m(ctx.data);
        result = expr_calculate(&expr->expression, ctx);
      } else {
        result = ExprValueErr(
            null,
            str_owned(
                "Variable '%$slice' is not const and cannot be calculated"));
      }
    } else {
      result = ExprValueErr(
          null, str_owned("Variable '%$slice' not found", var_name));
    }
  }

  return result;
}
// calc_backend_call_function

static int cb_get_expr_type(CalcBackend* this, const Expr* expr) {
  return calc_backend_get_expr_type(this, expr);
}

ExprContext calc_backend_get_context(CalcBackend* this) {
  static const ExprContextVtable table = {
      .is_variable = (void*)cb_is_variable,
      .is_function = (void*)cb_is_function,

      .get_variable_val = (void*)cb_get_variable_val,
      .call_function = (void*)calc_backend_call_function,

      .is_expr_const = (void*)calc_backend_is_expr_const,
      .get_expr_type = (void*)cb_get_expr_type,

      .get_variable_info = (void*)cb_get_variable_info,
      .get_function_info = (void*)cb_get_function_info,
  };
  return (ExprContext){.data = this, .vtable = &table};
}

ExprVariableInfo cb_get_variable_info(CalcBackend* this, StrSlice var_name) {
  debugln("Smone asks for variable '%$slice' info", var_name);
  CalcExpr* expr = calc_backend_get_variable_sslice(this, var_name);
  debugln("Got expr: %p", expr);
  CalcValue* val = calc_backend_get_value_sslice(this, var_name);
  debugln("Got val: %p", val);
  ExprContext ctx = calc_backend_get_var_context_sslice(this, var_name);
  debugln("Got ctx: data %p + vtable %p", ctx.data, ctx.vtable);

  if (not expr and not val) {
    if (this->parent) {
      return cb_get_variable_info(this->parent, var_name);
    } else {
      debugln("Not found parent, so it is null");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
      ExprVariableInfo res = {
          .is_const = false,
          .value = null,
          .value_type = VALUE_TYPE_UNKNOWN,
          .expression = null,
          .correct_context = null,
      };
#pragma GCC diagnostic pop
      return res;
    }
  }

  ExprVariableInfo result = {
      .expression = expr ? &expr->expression : null,
      .value = val ? &val->value : null,
      .is_const = val or calc_backend_is_var_const_sslice(this, var_name),
      .value_type = VALUE_TYPE_UNKNOWN,
      .correct_context = ctx,
  };
  return result;
}
ExprFunctionInfo cb_get_function_info(CalcBackend* this, StrSlice fun_name) {
  CalcExpr* expr = calc_backend_get_function_sslice(this, fun_name);

  if (not expr) {
    if (this->parent) {
      return cb_get_function_info(this->parent, fun_name);
    } else {
      ExprFunctionInfo result = {
          .is_const = false,
          .correct_context = {.data = null, .vtable = null},
          .value_type = VALUE_TYPE_UNKNOWN,
          .expression = null,
          .args_names = null,
      };
      return result;
    }
  }

  ExprFunctionInfo result = {
      .is_const = calc_backend_is_func_const_sslice(this, fun_name),
      .correct_context = calc_backend_get_fun_context_sslice(this, fun_name),
      .expression = expr ? &expr->expression : null,
      .value_type = VALUE_TYPE_UNKNOWN,
      .args_names = expr ? &expr->function.args : null,
  };
  return result;
}