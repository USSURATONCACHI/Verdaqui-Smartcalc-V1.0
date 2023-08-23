#include "glsl_compiler.h"

#include <limits.h>
#include <math.h>
#include <string.h>

#include "../calculator/func_const_ctx.h"
#include "../util/allocator.h"

static StrResult function_to_glsl(ExprContext ctx, GlslContext* glsl,
                                  const Expr* expr, const vec_str_t* used_args);
static StrResult operator_to_glsl(ExprContext ctx, GlslContext* glsl,
                                  const Expr* expr, const vec_str_t* used_args);
static StrResult variable_to_glsl(ExprContext ctx, GlslContext* glsl,
                                  const Expr* expr, const vec_str_t* used_args);

static str_t non_const_types_err_msg(ExprValue value, const Expr* expr);

StrResult glsl_compile_expression(ExprContext ctx, GlslContext* glsl,
                                  const Expr* expr,
                                  const vec_str_t* used_args) {
  assert_m(expr);
  // debugln("Compiling '%$expr'", *expr);
  FuncConstCtx fctx = {
      .parent = ctx,
      .used_args = (vec_str_t*)used_args,
      .are_const = false,
  };
  ExprContext local_ctx = func_const_ctx_context(&fctx);
  if (local_ctx.vtable->is_expr_const(local_ctx.data, expr)) {
    // Calculate and insert as-is
    ExprValueResult res = expr_calculate(expr, local_ctx);
    if (not res.is_ok) return StrErr(res.err_text);
    ExprValue value = res.ok;

    if (value.type != EXPR_VALUE_NUMBER) {
      return StrErr(non_const_types_err_msg(value, expr));
    } else {
      str_t result = isnan(value.number) ? str_literal("nan")
                                         : str_owned("%.10lf", value.number);
      expr_value_free(value);
      return StrOk(result);
    }
  } else {
    // Convert to GLSL expression
    if (expr->type is EXPR_VECTOR) {
      return StrErr(
          str_owned("Vectors cannon be used in plot expressions. And '%$expr' "
                    "is a vector.",
                    *expr));
    } else {
      switch (expr->type) {
        case EXPR_NUMBER:
          return StrOk(isnan(expr->number.value)
                           ? str_literal("nan")
                           : str_owned("%lf", expr->number.value));
        case EXPR_VARIABLE:
          return variable_to_glsl(local_ctx, glsl, expr, used_args);
        case EXPR_FUNCTION:
          return function_to_glsl(local_ctx, glsl, expr, used_args);
        case EXPR_BINARY_OP:
          return operator_to_glsl(local_ctx, glsl, expr, used_args);
        default:
          panic("Invalid expr type");
      }
    }
  }
}

// HELPERS

// =====
// VARIABLE TO GLSL
// =====

static StrResult variable_to_glsl_calculate_const(const char* var_name,
                                                  ExprVariableInfo info);
static StrResult variable_to_glsl_turn_to_fn(GlslContext* glsl,
                                             const char* var_name,
                                             ExprVariableInfo info);

static StrResult variable_to_glsl(ExprContext ctx, GlslContext* glsl,
                                  const Expr* expr,
                                  const vec_str_t* used_args) {
  const char* var_name = expr->variable.name.string;
  // debugln("Glsl var '%s'", var_name);
  StrSlice var_name_slice = str_slice_from_str_t(&expr->variable.name);

  StrResult result;

  bool contains = false;
  for (int i = 0; i < used_args->length and not contains; i++)
    if (strcmp(used_args->data[i].string, var_name) is 0) contains = true;

  if (contains) {
    result = StrOk(str_owned("arg_%s", var_name));
  } else if (ctx.vtable->is_variable(ctx.data, var_name_slice)) {
    debugln("Is actually variable (vtable is %p)", ctx.vtable);
    assert_m(ctx.vtable->get_variable_info);
    debug_push();

    debugln("Fn is %p", ctx.vtable->get_variable_info);
    ExprVariableInfo info =
        ctx.vtable->get_variable_info(ctx.data, var_name_slice);

    debug_pop();
    debugln("Got info");
    if (info.is_const) {
      // Calculate and insert value
      debugln("Const");
      result = variable_to_glsl_calculate_const(var_name, info);
    } else if (info.expression) {
      // Turn into var_ function of x, y
      debugln("Non const");
      result = variable_to_glsl_turn_to_fn(glsl, var_name, info);
    } else {
      panic("No available way to turn non-const variable into var_ function");
    }
  } else if (strcmp(var_name, "x") is 0) {
    result = StrOk(str_literal("pos.x"));
  } else if (strcmp(var_name, "y") is 0) {
    result = StrOk(str_literal("pos.y"));
  } else {
    result = StrErr(str_owned("Variable %s is not found", var_name));
  }

  return result;
}

static StrResult variable_to_glsl_calculate_const(const char* var_name,
                                                  ExprVariableInfo info) {
  ExprValueResult value;

  if (info.value)
    value = ExprValueOk(expr_value_clone(info.value));
  else if (info.expression)
    value = expr_calculate(info.expression, info.correct_context);
  else
    panic("No accessible way to compute const varaible!");

  StrResult result;
  if (value.is_ok) {
    if (info.value->type is EXPR_VALUE_NUMBER)
      result = StrOk(str_owned("%$expr_value", value.ok));
    else
      result = StrErr(str_owned(
          "Non-number constants (%s = %$expr_value) cannot be used in plots",
          var_name, value.ok));
    expr_value_free(value.ok);
  } else {
    result = StrErr(value.err_text);
  }

  return result;
}

static StrResult variable_to_glsl_turn_to_fn(GlslContext* glsl,
                                             const char* var_name,
                                             ExprVariableInfo info) {
  str_t glsl_var_fn_name = str_owned("var_%s", var_name);

  StrResult result = StrOk(str_literal("--garbage--"));
  if (glsl_context_get_function(glsl, glsl_var_fn_name.string)) {
    result = StrOk(str_owned("%s(pos, step)", glsl_var_fn_name.string));
    str_free(glsl_var_fn_name);
  } else {
    vec_str_t args = vec_str_t_create();
    StrResult code = glsl_compile_expression(info.correct_context, glsl,
                                             info.expression, &args);

    if (code.is_ok) {
      result = StrOk(str_owned("%s(pos, step)", glsl_var_fn_name.string));
      GlslFunction fn = {
          .name = glsl_var_fn_name,
          .args = args,
          .code = str_owned("return %s;", code.data.string),
      };
      str_free(code.data);
      glsl_context_add_function(glsl, fn);
    } else {
      str_free(glsl_var_fn_name);
      vec_str_t_free(args);
      result = code;
    }
  }

  return result;
}
// ====
// VARIABLE TO GLSL END
// ====

// =====
// FUNCTION TO GLSL
// =====
static int get_func_args_count(ExprContext ctx, const char* fn_name);
static bool is_func_glsl_native(const char* fn_name);
static str_t call_native_function(const char* native_fn, const char* argument);

static StrResult ftgl_check_correctness(ExprContext this, GlslContext* glsl,
                                        const Expr* expr,
                                        const vec_str_t* used_args);

static StrResult compile_function_to_glsl(ExprContext ctx, GlslContext* glsl,
                                          const Expr* function);
static StrResult glsl_compile_fn_args_values(ExprContext ctx, GlslContext* glsl,
                                             const Expr* fn_argument,
                                             const vec_str_t* used_args);

static StrResult function_to_glsl(ExprContext ctx, GlslContext* glsl,
                                  const Expr* expr,
                                  const vec_str_t* used_args) {
  assert_m(expr->type is EXPR_FUNCTION);
  StrResult result = ftgl_check_correctness(ctx, glsl, expr, used_args);
  if (not result.is_ok)
    return result;
  else
    str_free(result.data);

  StrResult argument = glsl_compile_fn_args_values(
      ctx, glsl, expr->function.argument, used_args);

  if (not argument.is_ok) {
    result = argument;
  } else {
    if (is_func_glsl_native(expr->function.name.string)) {
      result = StrOk(
          call_native_function(expr->function.name.string,
                               argument.data.string + 2));  // +2 to skip comma
    } else {
      str_t shader_func_name = str_owned("func_%s", expr->function.name.string);

      if (not glsl_context_get_function(glsl, shader_func_name.string))
        result = compile_function_to_glsl(ctx, glsl, expr);

      if (result.is_ok) {
        str_free(result.data);
        str_t tmp = str_owned("%s(pos, step%s)", shader_func_name.string,
                              argument.data.string);
        result = StrOk(tmp);
      }
      str_free(shader_func_name);
    }
    str_free(argument.data);
  }

  return result;
}

static StrResult glsl_compile_fn_args_values(ExprContext ctx, GlslContext* glsl,
                                             const Expr* fn_argument,
                                             const vec_str_t* used_args) {
  StringStream stream = string_stream_create();
  OutStream os = string_stream_stream(&stream);

  StrResult result = StrOk(str_literal(""));
  if (fn_argument->type is EXPR_VECTOR) {
    const vec_Expr* args_expr = &fn_argument->vector.arguments;

    for (int i = 0; i < args_expr->length and result.is_ok; i++) {
      StrResult local_res =
          glsl_compile_expression(ctx, glsl, &args_expr->data[i], used_args);
      if (not local_res.is_ok) {
        result = local_res;
      } else {
        x_sprintf(os, ", %s", local_res.data.string);
        str_free(local_res.data);
      }
    }
  } else {
    StrResult local_res =
        glsl_compile_expression(ctx, glsl, fn_argument, used_args);
    if (not local_res.is_ok)
      result = local_res;
    else {
      x_sprintf(os, ", %s", local_res.data.string);
      str_free(local_res.data);
    }
  }

  if (result.is_ok) {
    str_free(result.data);
    result = StrOk(string_stream_to_str_t(stream));
  } else {
    string_stream_free(stream);
  }

  return result;
}

static StrResult compile_function_to_glsl(ExprContext ctx, GlslContext* glsl,
                                          const Expr* expr) {
  ExprFunctionInfo info = ctx.vtable->get_function_info(
      ctx.data, str_slice_from_str_t(&expr->function.name));

  StrResult result;
  if (not info.expression) {
    result = StrErr(
        str_owned("Function '%s' not found", expr->function.name.string));
  } else {
    StrResult code = glsl_compile_expression(info.correct_context, glsl,
                                             info.expression, info.args_names);
    if (not code.is_ok) {
      result = StrErr(code.data);
    } else {
      GlslFunction func = {
          .args = vec_str_t_clone(info.args_names),
          .code = str_owned("return %s;", code.data.string),
          .name = str_owned("func_%s", expr->function.name.string)};

      glsl_context_add_function(glsl, func);
      str_free(code.data);

      result = StrOk(str_literal("-"));
    }
  }

  return result;
}

static StrResult ftgl_check_correctness(ExprContext ctx, GlslContext* glsl,
                                        const Expr* expr,
                                        const vec_str_t* used_args) {
  unused(glsl);
  unused(used_args);
  assert_m(expr->type is EXPR_FUNCTION);
  int required_args = get_func_args_count(ctx, expr->function.name.string);
  if (required_args < 0)
    return StrErr(
        str_owned("Function '%s' cannot be found", expr->function.name.string));

  int fn_args_count;
  if (expr->function.argument->type != EXPR_VECTOR) {
    fn_args_count = 1;
  } else {
    fn_args_count = expr->function.argument->vector.arguments.length;
  }

  if (fn_args_count != required_args)
    return StrErr(str_owned(
        "Function '%s' accepts %d arguments, but %d args were provided.",
        expr->function.name.string, required_args, fn_args_count));

  return StrOk(str_literal("Ok"));
}

static int get_func_args_count(ExprContext ctx, const char* fn_name) {
  if (is_func_glsl_native(fn_name)) return 1;

  ExprFunctionInfo info =
      ctx.vtable->get_function_info(ctx.data, str_slice_from_string(fn_name));
  if (info.args_names) {
    return info.args_names->length;
  } else {
    return -1;
  }
}
static bool is_func_glsl_native(const char* fn_name) {
  const char* const glsl_native[] = {"sin",  "cos", "asin", "acos", "tan",
                                     "atan", "ln",  "log",  "sqrt"};
  for (int i = 0; i < (int)LEN(glsl_native); i++)
    if (strcmp(fn_name, glsl_native[i]) is 0) return true;

  return false;
}

#define E "2.71828182846"
static str_t call_native_function(const char* native_fn, const char* argument) {
  if (strcmp(native_fn, "ln") is 0)
    return str_owned("log(%s)/log(" E ")", argument);
  else if (strcmp(native_fn, "log") is 0)
    return str_owned("log(%s)/log(10.0)", argument);
  else
    return str_owned("%s(%s)", native_fn, argument);
}
// =====
// FUNCTION TO GLSL END
// =====

// OTHER

static str_t non_const_types_err_msg(ExprValue value, const Expr* expr) {
  assert_m(value.type != EXPR_VALUE_NUMBER);
  int type = value.type;

  str_t message = str_owned(
      "Non-const values of type '%s' (value of '%$expr') are not supported. "
      "Expression '%$expr' has value of '%$expr_value' which cannot be plotted."
      "Plots only support pure numeric operations. Other types can only be "
      "used in XY-independent cases.",
      expr_value_type_text(type), *expr, *expr, value);

  expr_value_free(value);
  return message;
}

// + - * / ^ > < >= <= == = !=
#define cmp(a, b) strcmp((a), (b)) is 0

#define TemplateOperator(fn_name, ...)                                    \
  static StrResult fn_name##_operator(ExprContext ctx, GlslContext* glsl, \
                                      const Expr* expr,                   \
                                      const vec_str_t* used_args) {       \
    assert_m(expr->type is EXPR_BINARY_OP);                               \
                                                                          \
    StrResult left_r = glsl_compile_expression(                           \
        ctx, glsl, expr->binary_operator.lhs, used_args);                 \
    if (not left_r.is_ok) return left_r;                                  \
                                                                          \
    StrResult right_r = glsl_compile_expression(                          \
        ctx, glsl, expr->binary_operator.rhs, used_args);                 \
    if (not right_r.is_ok) {                                              \
      str_result_free(left_r);                                            \
      return right_r;                                                     \
    }                                                                     \
    const char* left = left_r.data.string;                                \
    const char* right = right_r.data.string;                              \
    const char* name = expr->binary_operator.name.string;                 \
    unused(name);                                                         \
    unused(left);                                                         \
    unused(right);                                                        \
    str_t result = str_owned(__VA_ARGS__);                                \
                                                                          \
    str_result_free(left_r);                                              \
    str_result_free(right_r);                                             \
    return StrOk(result);                                                 \
  }

TemplateOperator(classic, "(%s %s %s)", left, name, right)
    TemplateOperator(comparsion, "((%s %s %s) ? 1.0 : 0.0)", left, name, right)
        TemplateOperator(mod, "mod(%s, %s)", left, right)

            static StrResult
    equality_operator(ExprContext this, GlslContext* glsl, const Expr* expr,
                      const vec_str_t* used_args);
static StrResult powf_operator(ExprContext ctx, GlslContext* glsl,
                               const Expr* expr, const vec_str_t* used_args);

static StrResult operator_to_glsl(ExprContext this, GlslContext* glsl,
                                  const Expr* expr,
                                  const vec_str_t* used_args) {
  assert_m(expr->type is EXPR_BINARY_OP);

  const char* op_name = expr->binary_operator.name.string;

  if (cmp(op_name, "+") or cmp(op_name, "-") or cmp(op_name, "*") or
      cmp(op_name, "/")) {
    return classic_operator(this, glsl, expr, used_args);
  } else if (cmp(op_name, "^")) {
    return powf_operator(this, glsl, expr, used_args);
  } else if (cmp(op_name, "<") or cmp(op_name, ">") or cmp(op_name, "<=") or
             cmp(op_name, ">=")) {
    return comparsion_operator(this, glsl, expr, used_args);
  } else if (cmp(op_name, "==") or cmp(op_name, "!=") or cmp(op_name, "=")) {
    return equality_operator(this, glsl, expr, used_args);
  } else if (cmp(op_name, "%") or cmp(op_name, "mod")) {
    return mod_operator(this, glsl, expr, used_args);
  } else {
    return StrErr(
        str_owned("Operator '%s' cannot used in plot-expression", op_name));
  }
}

static int get_int(const char* text);

static StrResult powf_operator(ExprContext ctx, GlslContext* glsl,
                               const Expr* expr, const vec_str_t* used_args) {
  assert_m(expr->type is EXPR_BINARY_OP);

  StrResult left_r =
      glsl_compile_expression(ctx, glsl, expr->binary_operator.lhs, used_args);
  if (not left_r.is_ok) return left_r;

  StrResult right_r =
      glsl_compile_expression(ctx, glsl, expr->binary_operator.rhs, used_args);
  if (not right_r.is_ok) {
    str_result_free(left_r);
    return right_r;
  }
  const char* left = left_r.data.string;
  const char* right = right_r.data.string;
  str_t result;

  int int_val = get_int(right);
  if (int_val != INT_MAX and int_val >= -32 and int_val <= 32 and
      strlen(left) < 16) {
    StringStream stream = string_stream_create();
    OutStream os = string_stream_stream(&stream);
    x_sprintf(os, "(1.0");

    for (int i = 1; i <= int_val; i++) x_sprintf(os, "*%s", left);

    for (int i = -1; i >= int_val; i--) x_sprintf(os, "/%s", left);

    x_sprintf(os, ")");
    result = string_stream_to_str_t(stream);
  } else
    result = str_owned("pow(%s, %s)", left, right);

  str_result_free(left_r);
  str_result_free(right_r);
  return StrOk(result);
}

static int get_int(const char* text) {
  double num = 0.0;
  int offset = 0;
  int success = sscanf(text, "%lf%n", &num, &offset);

  if (success and offset > 0 and num is round(num)) {
    return (int)round(num);
  } else {
    return INT_MAX;
  }
}

static str_t eq_function_text(const char* fn_name, const char* used_args_text,
                              bool is_eq) {
  str_t res = str_owned(
      "float \n"
      "    lb = %s(pos + vec2(0.0, step.y), step%s), \n"
      "    rb = %s(pos + step, step%s), \n"
      "    lt = %s(pos, step%s), \n"
      "    rt = %s(pos + vec2(step.x, 0.0), step%s);\n"
      "\n"
      "bool res =\n"
      "    sign_changes(lt, rt) ||\n"
      "    sign_changes(lt, rb) ||\n"
      "    sign_changes(lt, lb) ||\n"
      "\n"
      "    sign_changes(lb, rb) ||\n"
      "    sign_changes(lb, rt) ||\n"
      "\n"
      "    sign_changes(rb, rt);\n"
      "return (%sres) ? 1.0 : 0.0;",
      fn_name, used_args_text, fn_name, used_args_text, fn_name, used_args_text,
      fn_name, used_args_text, is_eq ? " " : "!");
  return res;
}

static StrResult equality_operator(ExprContext ctx, GlslContext* glsl,
                                   const Expr* expr,
                                   const vec_str_t* used_args) {
  const char* op_name = expr->binary_operator.name.string;
  bool eq_or_neq;
  if (cmp(op_name, "==") or cmp(op_name, "="))
    eq_or_neq = true;
  else if (cmp(op_name, "!="))
    eq_or_neq = false;
  else
    panic("Invalid eq operator");

  StrResult left_r =
      glsl_compile_expression(ctx, glsl, expr->binary_operator.lhs, used_args);
  if (not left_r.is_ok) return left_r;

  StrResult right_r =
      glsl_compile_expression(ctx, glsl, expr->binary_operator.rhs, used_args);
  if (not right_r.is_ok) {
    str_result_free(left_r);
    return right_r;
  }

  str_t expr_function_name = glsl_context_get_unique_fn_name(glsl);
  GlslFunction fn = {
      .name = str_clone(&expr_function_name),
      .args = vec_str_t_clone(used_args),
      .code = str_owned("return (%s) - (%s);", left_r.data.string,
                        right_r.data.string),
  };
  str_result_free(left_r);
  str_result_free(right_r);
  glsl_context_add_function(glsl, fn);

  str_t args_text = glsl_args_vals_to_string(used_args);
  str_t expr_change_fn_name = glsl_context_get_unique_fn_name(glsl);
  GlslFunction fn2 = {.name = str_clone(&expr_change_fn_name),
                      .args = vec_str_t_clone(used_args),
                      .code = eq_function_text(expr_function_name.string,
                                               args_text.string, eq_or_neq)};
  glsl_context_add_function(glsl, fn2);
  str_free(expr_function_name);

  str_t result = str_owned("%s(pos, step%s)", expr_change_fn_name.string,
                           args_text.string);

  str_free(expr_change_fn_name);
  str_free(args_text);
  return StrOk(result);
}
