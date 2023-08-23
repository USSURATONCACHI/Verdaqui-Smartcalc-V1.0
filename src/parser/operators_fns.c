#include "operators_fns.h"

#include <math.h>
#include <string.h>

#include "../util/prettify_c.h"

#define OPERATORS_FUNCS                                                   \
  {                                                                       \
    expr_operator_mod, expr_operator_range, expr_operator_range_included, \
                                                                          \
        expr_operator_assign, expr_operator_add_assign,                   \
        expr_operator_sub_assign, expr_operator_mul_assign,               \
        expr_operator_div_assign, expr_operator_mod_assign,               \
        expr_operator_pow_assign,                                         \
                                                                          \
        expr_operator_eq, expr_operator_neq, expr_operator_lte,           \
        expr_operator_gte, expr_operator_lt, expr_operator_gt,            \
                                                                          \
        expr_operator_eq, expr_operator_add, expr_operator_sub,           \
        expr_operator_mul, expr_operator_div, expr_operator_mod,          \
        expr_operator_pow,                                                \
                                                                          \
        expr_operator_index,                                              \
  }

OperatorFn expr_get_operator_fn_slice(StrSlice name) {
  const char* const operators[] = {
      "mod",                                       // Mod
      "..",  "..=",                                // Range
      ":=",  "+=",  "-=", "*=", "/=", "%=", "^=",  // Procedures
      "==",  "!=",  "<=", ">=", "<",  ">",         // Comparsions
      "=",   "+",   "-",  "*",  "/",  "%",  "^",   // Operators
      "[]",
  };

  const OperatorFn funcs[] = OPERATORS_FUNCS;

  int len = LEN(operators);
  assert_m(LEN(operators) == LEN(funcs));

  for (int i = 0; i < len; i++) {
    if (name.length == (int)strlen(operators[i]) and
        strncmp(name.start, operators[i], name.length) is 0)
      return funcs[i];
  }
  return null;
}

OperatorFn expr_get_operator_fn(const char* name) {
  return expr_get_operator_fn_slice(
      (StrSlice){.start = name, .length = strlen(name)});
}

#define Err(...)                                                        \
  (ExprValueResult) {                                                   \
    .is_ok = false, .err_pos = null, .err_text = str_owned(__VA_ARGS__) \
  }
#define Ok(val) \
  (ExprValueResult) { .is_ok = true, .ok = (val) }

static ExprValueResult template_alg_operator(ExprValue* a, ExprValue* b,
                                             double (*fn)(double, double),
                                             const char* err_name);

static ExprValueResult template_alg_vecvec(ExprValue* a, ExprValue* b,
                                           double (*fn)(double, double),
                                           const char* err_name) {
  //.
  assert_m(a->type is EXPR_VALUE_VEC and b->type is EXPR_VALUE_VEC);

  if (a->vec.length != b->vec.length)
    return Err(
        "Elements of vectors of different lengths (%d vs %d) cannot be %s",
        a->vec.length, b->vec.length, err_name);

  ExprValueResult result = {.is_ok = true};

  vec_ExprValue values = vec_ExprValue_with_capacity(a->vec.length);

  for (int i = 0; i < a->vec.length and result.is_ok; i++) {
    result =
        template_alg_operator(&a->vec.data[i], &b->vec.data[i], fn, err_name);

    if (result.is_ok) vec_ExprValue_push(&values, result.ok);
  }

  if (result.is_ok)
    result.ok = (ExprValue){
        .type = EXPR_VALUE_VEC,
        .vec = values,
    };
  else
    vec_ExprValue_free(values);

  return result;
}

static ExprValueResult template_alg_vecnum(ExprValue* a, ExprValue* b,
                                           double (*fn)(double, double),
                                           const char* err_name) {
  ExprValue* vec = a;
  ExprValue* number = b;
  bool is_inverted = false;

  if (vec->type is EXPR_VALUE_NUMBER) {
    SWAP(ExprValue*, vec, number);
    is_inverted = true;
  }

  assert_m(vec->type is EXPR_VALUE_VEC and number->type is EXPR_VALUE_NUMBER);

  ExprValueResult result = {.is_ok = true};
  vec_ExprValue values = vec_ExprValue_with_capacity(vec->vec.length);

  for (int i = 0; i < vec->vec.length and result.is_ok; i++) {
    if (not is_inverted)
      result = template_alg_operator(&vec->vec.data[i], number, fn, err_name);
    else
      result = template_alg_operator(number, &vec->vec.data[i], fn, err_name);

    if (result.is_ok) vec_ExprValue_push(&values, result.ok);
  }

  if (result.is_ok)
    result.ok = (ExprValue){
        .type = EXPR_VALUE_VEC,
        .vec = values,
    };
  else
    vec_ExprValue_free(values);

  return result;
}
static ExprValueResult template_alg_operator(ExprValue* a, ExprValue* b,
                                             double (*fn)(double, double),
                                             const char* err_name) {
  //.
  ExprValueResult result = {.is_ok = true};
  if (a->type is EXPR_VALUE_NONE or b->type is EXPR_VALUE_NONE) {
    result = Err("None values cannot be %s", err_name);
  } else if (a->type is EXPR_VALUE_NUMBER and b->type is EXPR_VALUE_NUMBER) {
    ExprValue v = {.type = EXPR_VALUE_NUMBER,
                   .number = fn(a->number, b->number)};
    result = Ok(v);
  } else if (a->type is EXPR_VALUE_VEC and b->type is EXPR_VALUE_VEC) {
    result = template_alg_vecvec(a, b, fn, err_name);
  } else {
    result = template_alg_vecnum(a, b, fn, err_name);
  }

  return result;
}

#define AlgOperator(name, err_name, action)                         \
  static double expr_operator_##name##_lambda(double a, double b) { \
    return action;                                                  \
  }                                                                 \
  ExprValueResult expr_operator_##name(ExprValue a, ExprValue b) {  \
    ExprValueResult res = template_alg_operator(                    \
        &a, &b, expr_operator_##name##_lambda, err_name);           \
    expr_value_free(a);                                             \
    expr_value_free(b);                                             \
    return res;                                                     \
  }

AlgOperator(add, "added", a + b) AlgOperator(sub, "subtracted", a - b)
    AlgOperator(mul, "multiply", a* b) AlgOperator(div, "divide", a / b)
        AlgOperator(mod, "mod-ded", fmod(a, b))
            AlgOperator(pow, "exponentiated", pow(a, b))

                static long long check_num_integer(double number,
                                                   ExprValueResult* res);

ExprValueResult expr_operator_index(ExprValue a, ExprValue b) {
  ExprValueResult result = {.is_ok = true};

  if (a.type is EXPR_VALUE_VEC) {
    if (b.type is EXPR_VALUE_NUMBER) {
      long long index = check_num_integer(b.number, &result);
      if (result.is_ok) {
        if (index >= 0 and index < (long long)a.vec.length) {
          result = (ExprValueResult){
              .is_ok = true,
              .ok = expr_value_clone(&a.vec.data[index]),
          };
        } else {
          result = Err("Index %lld is out of bounds for vector of length %d",
                       index, a.vec.length);
        }
      }
    } else {
      result = Err("Index is not a number (it is '%s' instead)",
                   expr_value_type_text(b.type));
    }
  } else {
    result = Err("Indexing subject is not vector (it is '%s' instead)",
                 expr_value_type_text(a.type));
  }

  expr_value_free(a);
  expr_value_free(b);
  return result;
}

#undef Err
//
//
//
//
//
// RANGES
#define MAX_RANGE 10000
#define Err(...)                                                        \
  (ExprValueResult) {                                                   \
    .is_ok = false, .err_pos = null, .err_text = str_owned(__VA_ARGS__) \
  }

#define EPSILON 0.0001

static long long check_num_integer(double number, ExprValueResult* res) {
  int result = 0;
  if (fabs(round(number) - number) > EPSILON) {
    (*res) = Err("Range error: number %lf is not an integer (error of +-" STR(
                     EPSILON) " from integer value is allowed)",
                 number);
  } else {
    result = (long)round(number);
  }
  return result;
}

static ExprValueResult expr_op_range_template(ExprValue a, ExprValue b,
                                              bool inclusive) {
  ExprValueResult result = {.is_ok = true};
  if (a.type is EXPR_VALUE_NUMBER) {
    if (b.type is EXPR_VALUE_NUMBER) {
      long long low = check_num_integer(a.number, &result);
      if (result.is_ok) {
        long long high = check_num_integer(b.number, &result);
        if (result.is_ok) {
          long long len = high > low ? high - low : 0;
          debugln("Len: %lld", len);
          vec_ExprValue vec = vec_ExprValue_with_capacity(len);

          for (long long i = low; inclusive ? i <= high : i < high; i++)
            vec_ExprValue_push(&vec, (ExprValue){.type = EXPR_VALUE_NUMBER,
                                                 .number = (double)i});

          result = (ExprValueResult){.is_ok = true,
                                     .ok = {
                                         .type = EXPR_VALUE_VEC,
                                         .vec = vec,
                                     }};
        }
      }
    } else
      result = Err("Range lower bound is not a number (it has type of '%s')",
                   expr_value_type_text(a.type));
  } else
    result = Err("Range lower bound is not a number (it has type of '%s')",
                 expr_value_type_text(a.type));

  expr_value_free(a);
  expr_value_free(b);

  return result;
}
#undef Err

ExprValueResult expr_operator_range(ExprValue a, ExprValue b) {
  debugln("Starting to range");
  return expr_op_range_template(a, b, false);
}
ExprValueResult expr_operator_range_included(ExprValue a, ExprValue b) {
  return expr_op_range_template(a, b, true);
}

//
//
//
// ASSIGNS
ExprValueResult expr_operator_assign(ExprValue a, ExprValue b) {
  unused(a);
  return (ExprValueResult){.is_ok = true, .ok = b};
}
ExprValueResult expr_operator_add_assign(ExprValue a, ExprValue b) {
  unused(a);
  return (ExprValueResult){.is_ok = true, .ok = b};
}
ExprValueResult expr_operator_sub_assign(ExprValue a, ExprValue b) {
  unused(a);
  return (ExprValueResult){.is_ok = true, .ok = b};
}
ExprValueResult expr_operator_mul_assign(ExprValue a, ExprValue b) {
  unused(a);
  return (ExprValueResult){.is_ok = true, .ok = b};
}
ExprValueResult expr_operator_div_assign(ExprValue a, ExprValue b) {
  unused(a);
  return (ExprValueResult){.is_ok = true, .ok = b};
}
ExprValueResult expr_operator_mod_assign(ExprValue a, ExprValue b) {
  unused(a);
  return (ExprValueResult){.is_ok = true, .ok = b};
}
ExprValueResult expr_operator_pow_assign(ExprValue a, ExprValue b) {
  unused(a);
  return (ExprValueResult){.is_ok = true, .ok = b};
}

//
//
//
//
// COMPARSIONS
static bool expr_operator_eq_ptr(ExprValue* a, ExprValue* b) {
  if (a->type != b->type) return false;

  if (a->type is EXPR_VALUE_NONE)
    return true;
  else if (a->type is EXPR_VALUE_NUMBER)
    return a->number == b->number;
  else if (a->type is EXPR_VALUE_VEC) {
    if (a->vec.length != b->vec.length) return false;

    for (int i = 0; i < a->vec.length; i++)
      if (not expr_operator_eq_ptr(&a->vec.data[i], &b->vec.data[i]))
        return false;

    return true;
  } else {
    panic("Unknown ExprValue type: %d", a->type);
  }
}

#define Number(cond)               \
  (ExprValueResult) {              \
    .is_ok = true, .ok = {         \
      .type = EXPR_VALUE_NUMBER,   \
      .number = (cond) ? 1.0 : 0.0 \
    }                              \
  }
ExprValueResult expr_operator_eq(ExprValue a, ExprValue b) {
  bool result = expr_operator_eq_ptr(&a, &b);
  expr_value_free(a);
  expr_value_free(b);
  return Number(result);
}
ExprValueResult expr_operator_neq(ExprValue a, ExprValue b) {
  bool result = not expr_operator_eq_ptr(&a, &b);
  expr_value_free(a);
  expr_value_free(b);
  return Number(result);
}

#undef Number
#define Number(cond) \
  (ExprValue) { .type = EXPR_VALUE_NUMBER, .number = (cond) ? 1.0 : 0.0 }

ExprValue expr_comparsion_template(ExprValue* a, ExprValue* b,
                                   bool (*fn)(double, double)) {
  if (a->type != b->type) return Number(false);

  if (a->type is EXPR_VALUE_NONE)
    return Number(fn(0.0, 0.0));
  else if (a->type is EXPR_VALUE_NUMBER)
    return Number(fn(a->number, b->number));
  else if (a->type is EXPR_VALUE_VEC) {
    if (a->vec.length != b->vec.length) return Number(false);

    vec_ExprValue values = vec_ExprValue_with_capacity(a->vec.length);
    for (int i = 0; i < a->vec.length; i++) {
      ExprValue val =
          expr_comparsion_template(&a->vec.data[i], &b->vec.data[i], fn);
      vec_ExprValue_push(&values, val);
    }
    return (ExprValue){.type = EXPR_VALUE_VEC, .vec = values};
  } else
    panic("Unknown ExprValue type: %d", a->type);
}

#define Comparsion(name, action)                                         \
  static bool expr_operator_##name##_lambda(double a, double b) {        \
    return action;                                                       \
  }                                                                      \
  ExprValueResult expr_operator_##name(ExprValue a, ExprValue b) {       \
    ExprValue result =                                                   \
        expr_comparsion_template(&a, &b, expr_operator_##name##_lambda); \
    expr_value_free(a);                                                  \
    expr_value_free(b);                                                  \
    return (ExprValueResult){.is_ok = true, .ok = result};               \
  }

Comparsion(lte, a <= b) Comparsion(gte, a >= b) Comparsion(lt, a < b)
    Comparsion(gt, a > b)
#undef Number