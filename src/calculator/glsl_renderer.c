
#include "glsl_renderer.h"

#include "../util/allocator.h"
#include "../util/better_string.h"
#include "../util/prettify_c.h"

#define VECTOR_C GlslFunction
#define VECTOR_ITEM_DESTRUCTOR glsl_function_free
#include "../util/vector.h"  // vec_GlslFunction

static vec_str_t vec_str_t_clone(vec_str_t* source) {
  vec_str_t result = vec_str_t_with_capacity(source->length);

  for (int i = 0; i < source->length; i++)
    vec_str_t_push(&result, str_clone(&source->data[i]));

  return result;
}

GlslContext glsl_context_create() {
  return (GlslContext){
      .functions = vec_GlslFunction_create(),
  };
}
void glsl_context_free(GlslContext this) {
  vec_GlslFunction_free(this.functions);
}

#define Err(e) \
  (StrResult) { .is_ok = false, .data = e }

#define Ok(e) \
  (StrResult) { .is_ok = true, .data = e }

static str_t non_const_types_err_msg(ExprValue value, Expr* expr) {
  assert_m(value.type != EXPR_VALUE_NUMBER);
  int type = value.type;
  str_t value_text = str_raw_owned(expr_value_str_print(&value, null));
  str_t expr_text = str_raw_owned(expr_str_print(expr, null));
  expr_value_free(value);

  str_t message = str_owned(
      "Non-const values of type '%s' (value of '%s') are not supported. "
      "Expression '%s' has value of '%s' which cannot be plotted."
      "Plots only support pure numeric operations. Other types can only be "
      "used in XY-independent cases.",
      expr_value_type_text(type), expr_text.string, expr_text.string,
      value_text.string);

  str_free(value_text);
  str_free(expr_text);
  return message;
}

static bool is_func_glsl_native(const char* fn_name) {
  const char* const glsl_native[] = {"sin",  "cos", "asin", "acos", "tan",
                                     "atan", "ln",  "log",  "sqrt"};
  for (int i = 0; i < (int)LEN(glsl_native); i++)
    if (strcmp(fn_name, glsl_native[i]) is 0) return true;

  return false;
}

static int get_func_args_count(CalcBackend* this, const char* fn_name) {
  if (is_func_glsl_native(fn_name)) return 1;

  CalcExpr* func = calc_backend_get_function(this, fn_name);
  if (func) {
    return func->function.args.length;
  } else {
    return -1;
  }
}

static StrResult function_to_glsl(CalcBackend* this, GlslContext* glsl,
                                  Expr* expr, vec_str_t* used_args);
static StrResult operator_to_glsl(CalcBackend* this, GlslContext* glsl,
                                  Expr* expr, vec_str_t* used_args);
static StrResult variable_to_glsl(CalcBackend* this, GlslContext* glsl,
                                  Expr* expr, vec_str_t* used_args);

#define debug_exprln(before, expr, ...) \
  {                                     \
    debug(before);                      \
    expr_print((expr), DEBUG_OUT);      \
    debugc(__VA_ARGS__);                \
    debugc("\n");                       \
  }

StrResult calc_backend_to_glsl_code(CalcBackend* this, GlslContext* glsl,
                                    Expr* expr, vec_str_t* used_args) {
  assert_m(expr);
  // debugln("--Starting OPERATOR %p %s", expr, expr.);
  debug_exprln("==Starting to calculate '", expr, "'");

  debugln("==Lets see if its const...");
  if (calc_backend_is_const_expr(this, expr)) {
    debugln("== It IS const!");
    // Calculate and insert as-is
    ExprValueResult res =
        expr_calculate_val(expr, calc_backend_get_context(this));
    if (not res.is_ok) return Err(res.err_text);
    ExprValue value = res.ok;

    if (value.type != EXPR_VALUE_NUMBER) {
      return Err(non_const_types_err_msg(value, expr));
    } else {
      str_t value_text = str_raw_owned(expr_value_str_print(&value, null));
      expr_value_free(value);
      return Ok(value_text);
    }
  } else {
    debugln("Is is NOT const! It has a type of %d.", expr->type);
    // Convert to GLSL expression
    if (expr->type is EXPR_VECTOR) {
      str_t expr_text = str_raw_owned(expr_str_print(expr, null));
      str_t err = str_owned(
          "Vectors cannon be used in plot expressions. And '%s' is a vector.",
          expr_text.string);
      str_free(expr_text);
      return Err(err);

    } else {
      switch (expr->type) {
        case EXPR_NUMBER:
          return Ok(str_owned("%lf", expr->number.value));

        case EXPR_VARIABLE:
          return variable_to_glsl(this, glsl, expr, used_args);

        case EXPR_FUNCTION:
          return function_to_glsl(this, glsl, expr, used_args);

        case EXPR_BINARY_OP:
          return operator_to_glsl(this, glsl, expr, used_args);

        default:
          panic("Invalid expr type");
      }
    }
  }
}

static str_t append(str_t old, str_t append, const char* after) {
  str_t result = str_owned("%s%s%s", old.string, append.string, after);
  str_free(old);
  str_free(append);
  return result;
}

// =====
// FUNCTION TO GLSL
// =====
static StrResult function_to_glsl_check_correctness(CalcBackend* this,
                                                    GlslContext* glsl,
                                                    Expr* expr,
                                                    vec_str_t* used_args);
static StrResult function_to_glsl_collect_call_arg(CalcBackend* this,
                                                   GlslContext* glsl,
                                                   Expr* expr,
                                                   vec_str_t* used_args);
static StrResult function_to_glsl_parse(CalcBackend* this, GlslContext* glsl,
                                        Expr* call_expr, vec_str_t* used_args,
                                        CalcExpr* function_expr);

static StrResult function_to_glsl(CalcBackend* this, GlslContext* glsl,
                                  Expr* expr, vec_str_t* used_args) {
  StrResult result =
      function_to_glsl_check_correctness(this, glsl, expr, used_args);
  if (not result.is_ok)
    return result;
  else
    str_free(result.data);

  if (not is_func_glsl_native(expr->function.name.string)) {
    str_t shader_func_name = str_owned("func_%s", expr->function.name.string);
    if (not glsl_context_get_function(glsl, shader_func_name.string)) {
      CalcExpr* function =
          calc_backend_get_function(this, expr->function.name.string);

      if (function) {
        debugln("FUNCTION FUNCTION FUNCTION");
        CalcBackend function_ctx = {.parent = this,
                                    .values = vec_CalcValue_create(),
                                    .expressions = vec_CalcExpr_create()};

        Expr* arg = expr->function.argument;
        assert_m(arg);
        vec_CalcExpr args = vec_CalcExpr_create();
        if (arg->type is EXPR_VECTOR) {
        } else {
        }
        for (int i = 0; i < function->function.args.length; i++)
          debugln("Function %s arg %s", expr->function.name.string,
                  function->function.args.data[i].string);

        StrResult code = calc_backend_to_glsl_code(
            this, glsl, &function->expression, &function->function.args);
        if (code.is_ok) {
          GlslFunction fn = {
              .name = shader_func_name,
              .args = vec_str_t_clone(&function->function.args),
              .code = str_owned("return %s;", code.data.string),
          };
          str_free(code.data);
          glsl_context_add_function(glsl, fn);
        } else {
          str_free(shader_func_name);
          result = code;
        }
      } else {
        str_free(shader_func_name);
        result = Err(
            str_owned("Function '%s' not found", expr->function.name.string));
      }
    } else {
      str_free(shader_func_name);
    }
  }

  if (result.is_ok) {
    str_free(result.data);
    result = function_to_glsl_collect_call_arg(this, glsl, expr, used_args);
  }
  return result;
}

static StrResult function_to_glsl_parse(CalcBackend* this, GlslContext* glsl,
                                        Expr* call_expr, vec_str_t* used_args,
                                        CalcExpr* function_expr) {
  assert_m(call_expr->type is EXPR_FUNCTION);
  assert_m(function_expr->type is CALC_EXPR_FUNCTION);

  if (call_expr->type is EXPR_VECTOR) {
  } else {
  }
}

static StrResult function_to_glsl_collect_call_arg(CalcBackend* this,
                                                   GlslContext* glsl,
                                                   Expr* expr,
                                                   vec_str_t* used_args) {
  str_t result;

  if (is_func_glsl_native(expr->function.name.string))
    result = str_owned("%s(", expr->function.name.string);
  else
    result = str_owned("func_%s(pos, step, ", expr->function.name.string);

  if (expr->function.argument->type is EXPR_VECTOR) {
    vec_Expr* args = &expr->function.argument->vector.arguments;

    for (int i = 0; i < args->length; i++) {
      StrResult argument_text =
          calc_backend_to_glsl_code(this, glsl, &args->data[i], used_args);

      if (not argument_text.is_ok) {
        str_free(result);
        return argument_text;
      }

      result = append(result, argument_text.data,
                      (i < (args->length - 1)) ? ", " : ")");
    }
  } else {
    StrResult argument_text = calc_backend_to_glsl_code(
        this, glsl, expr->function.argument, used_args);
    if (not argument_text.is_ok) {
      str_free(result);
      return argument_text;
    }

    result = append(result, argument_text.data, ")");
  }
  return Ok(result);
}

static StrResult function_to_glsl_check_correctness(CalcBackend* this,
                                                    GlslContext* glsl,
                                                    Expr* expr,
                                                    vec_str_t* used_args) {
  assert_m(expr->type is EXPR_FUNCTION);
  int required_args = get_func_args_count(this, expr->function.name.string);
  if (required_args < 0)
    return Err(
        str_owned("Function '%s' cannot be plotted (only pure numeric and "
                  "existing functions can be "
                  "used in plot)",
                  expr->function.name.string));

  int fn_args_count;
  if (expr->function.argument->type != EXPR_VECTOR) {
    fn_args_count = 1;
  } else {
    fn_args_count = expr->function.argument->vector.arguments.length;
  }

  if (fn_args_count != required_args)
    return Err(str_owned(
        "Function '%s' accepts %d arguments, but %d args were provided.",
        expr->function.name.string, required_args, fn_args_count));

  return Ok(str_literal("Ok"));
}
// =====
// FUNCTION TO GLSL END
// =====
// VARIABLE TO GLSL
// =====

static StrResult variable_to_glsl(CalcBackend* this, GlslContext* glsl,
                                  Expr* expr, vec_str_t* used_args) {
  const char* var_name = expr->variable.name.string;

  StrResult result;
  CalcValue* value = calc_backend_get_value(this, var_name);
  CalcExpr* variable = calc_backend_get_variable(this, var_name);
  if (value) {
    assert_m(value->value.type is EXPR_VALUE_NUMBER);
    result = Ok(str_raw_owned(expr_value_str_print(&value->value, null)));
  } else if (variable) {
    str_t glsl_var_fn_name = str_owned("var_%s", var_name);

    if (glsl_context_get_function(glsl, glsl_var_fn_name.string)) {
      result = Ok(str_owned("%s(pos, step)", glsl_var_fn_name.string));
      str_free(glsl_var_fn_name);
    } else {
      vec_str_t args = vec_str_t_create();
      StrResult code =
          calc_backend_to_glsl_code(this, glsl, &variable->expression, &args);

      if (code.is_ok) {
        result = Ok(str_owned("%s(pos, step)", glsl_var_fn_name.string));
        GlslFunction fn = {
            .name = glsl_var_fn_name,
            .args = args,
            .code = str_owned("return %s;", code.data.string),
        };
        str_free(code.data);
        glsl_context_add_function(glsl, fn);
      } else {
        result = code;
      }
    }
  } else {
    bool contains = false;
    for (int i = 0; i < used_args->length and not contains; i++) {
      debugln("Cmp of '%s' and '%s' is %d", used_args->data[i].string, var_name,
              strcmp(used_args->data[i].string, var_name));
      if (strcmp(used_args->data[i].string, var_name) is 0) contains = true;
    }

    if (contains)
      result = Ok(str_owned("%s", var_name));
    else if (strcmp(var_name, "x") is 0)
      result = Ok(str_literal("pos.x"));
    else if (strcmp(var_name, "y") is 0)
      result = Ok(str_literal("pos.y"));
    else {
      result = Err(str_owned("Variable %s is not found", var_name));
    }
  }

  return result;
}

// ====
// VARIABLE TO GLSL END
// ====

// + - * / ^ > < >= <= == = !=
#define cmp(a, b) strcmp((a), (b)) is 0

#define TemplateOperator(fn_name, ...)                                       \
  static StrResult fn_name##_operator(CalcBackend* this, GlslContext* glsl,  \
                                      Expr* expr, vec_str_t* used_args) {    \
    assert_m(expr->type is EXPR_BINARY_OP);                                  \
    StrResult left_r = calc_backend_to_glsl_code(                            \
        this, glsl, expr->binary_operator.lhs, used_args);                   \
    if (not left_r.is_ok) return left_r;                                     \
    debugln("Blud %p MIGHT HAVE LEFT %s", expr, left_r.data.string);         \
    StrResult right_r = calc_backend_to_glsl_code(                           \
        this, glsl, expr->binary_operator.rhs, used_args);                   \
    if (not right_r.is_ok) {                                                 \
      str_result_free(left_r);                                               \
      return right_r;                                                        \
    }                                                                        \
    const char* left = left_r.data.string;                                   \
    const char* right = right_r.data.string;                                 \
    debugln("Blud MIGHT have these as its args - left: '%s' and right '%s'", \
            left, right);                                                    \
    const char* name = expr->binary_operator.name.string;                    \
    unused(name);                                                            \
    unused(left);                                                            \
    unused(right);                                                           \
    str_t result = str_owned(__VA_ARGS__);                                   \
    debugln("Blud %s MIGHT have got this: '%s'", name, result.string);       \
                                                                             \
    str_result_free(left_r);                                                 \
    str_result_free(right_r);                                                \
    return Ok(result);                                                       \
  }

TemplateOperator(classic, "(%s %s %s)", left, name, right);
TemplateOperator(powf, "pow(%s, %s)", left, right);
TemplateOperator(comparsion, "((%s %s %s) ? 1.0 : 0.0)", left, name, right);

static StrResult equality_operator(CalcBackend* this, GlslContext* glsl,
                                   Expr* expr, vec_str_t* used_args);

static StrResult operator_to_glsl(CalcBackend* this, GlslContext* glsl,
                                  Expr* expr, vec_str_t* used_args) {
  assert_m(expr->type is EXPR_BINARY_OP);
  debugln("Blud might be an OPERATOR. PLUH");

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
    // panic("Not yet implemented!");
    debugln("Blud might be an EQUALITY OP. PLUH");
    StrResult r = equality_operator(this, glsl, expr, used_args);
    return r;
  } else {
    return Err(
        str_owned("Operator '%s' cannot used in plot-expression", op_name));
  }
}

static str_t collect_args_to_text(vec_str_t* used_args) {
  int sum_lengths = strlen(", float ") * used_args->length;
  for (int i = 0; i < used_args->length; i++)
    sum_lengths += strlen(used_args->data[i].string);

  char* buffer = (char*)MALLOC(sizeof(char) * (sum_lengths + 1));

  char* write_pos = buffer;
  for (int i = 0; i < used_args->length; i++) {
    sprintf(write_pos, ", float %s", used_args->data[i].string);
    write_pos += strlen(", float ") + strlen(used_args->data[i].string);
  }
  write_pos[0] = '\0';
  assert_m(write_pos <= (buffer + sum_lengths));

  return str_raw_owned(buffer);
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

static StrResult equality_operator(CalcBackend* this, GlslContext* glsl,
                                   Expr* expr, vec_str_t* used_args) {
  const char* op_name = expr->binary_operator.name.string;
  bool eq_or_neq;
  if (cmp(op_name, "==") or cmp(op_name, "="))
    eq_or_neq = true;
  else if (cmp(op_name, "!="))
    eq_or_neq = false;
  else
    panic("Invalid eq operator");

  StrResult left_r = calc_backend_to_glsl_code(
      this, glsl, expr->binary_operator.lhs, used_args);
  if (not left_r.is_ok) return left_r;
  StrResult right_r = calc_backend_to_glsl_code(
      this, glsl, expr->binary_operator.rhs, used_args);
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

  str_t args_text = collect_args_to_text(used_args);
  str_t expr_change_fn_name = glsl_context_get_unique_fn_name(glsl);
  GlslFunction fn2 = {.name = str_clone(&expr_change_fn_name),
                      .args = vec_str_t_clone(used_args),
                      .code = eq_function_text(expr_function_name.string,
                                               args_text.string, eq_or_neq)};
  glsl_context_add_function(glsl, fn2);
  str_free(expr_function_name);

  str_t result = str_owned("%s(pos, step%s)", expr_change_fn_name.string,
                           args_text.string);
  debugln("EQUALITY OPERATOR RETURNS STR T %d %p", result.is_owned,
          result.string);

  str_free(expr_change_fn_name);
  str_free(args_text);
  return Ok(result);
}

void str_result_free(StrResult this) { str_free(this.data); }

void glsl_function_free(GlslFunction this) {
  str_free(this.name);
  str_free(this.code);
  vec_str_t_free(this.args);
}

str_t glsl_context_get_unique_fn_name(GlslContext* this) {
  int funcs_count = this->functions.length;
  int attempt = 0;

  while (true) {
    str_t name = str_owned("uniq_%d_%d", funcs_count, attempt);

    if (glsl_context_get_function(this, name.string)) {
      attempt++;
      str_free(name);
    } else {
      return name;
    }
  }
}

void glsl_context_add_function(GlslContext* this, GlslFunction fn) {
  if (glsl_context_get_function(this, fn.name.string))
    panic("Function %s was already added!", fn.name.string);

  vec_GlslFunction_push(&this->functions, fn);
}

GlslFunction* glsl_context_get_function(GlslContext* this,
                                        const char* fn_name) {
  for (int i = 0; i < this->functions.length; i++)
    if (strcmp(this->functions.data[i].name.string, fn_name) is 0)
      return &this->functions.data[i];

  return null;
}

str_t glsl_function_to_text(GlslFunction* this) {
  str_t args_text = collect_args_to_text(&this->args);
  str_t result =
      str_owned("float %s(vec2 pos, vec2 step%s) {\n%s\n}", this->name.string,
                args_text.string, this->code.string);
  str_free(args_text);
  return result;
}

str_t glsl_context_get_all_functions(GlslContext* this) {
  str_t result = str_literal("");

  debugln("GLSL this*: %p", this);
  for (int i = 0; i < this->functions.length; i++) {
    debugln("Function %d is '%s'", i, this->functions.data[i].name.string);
    str_t function = glsl_function_to_text(&this->functions.data[i]);
    debugln("Its text: '%s'", function.string);
    str_t tmp = str_owned("%s\n\n%s", result.string, function.string);
    str_free(function);
    str_free(result);
    result = tmp;
  }
  debugln("Result: '%s'", result.string);
  return result;
}
