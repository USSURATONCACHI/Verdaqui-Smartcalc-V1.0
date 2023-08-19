#include "native_functions.h"

#include <float.h>
#include <math.h>

#include "../util/allocator.h"

static ExprValueResult calculator_func_cos(vec_ExprValue args);
static ExprValueResult calculator_func_sin(vec_ExprValue args);
static ExprValueResult calculator_func_tan(vec_ExprValue args);
static ExprValueResult calculator_func_acos(vec_ExprValue args);
static ExprValueResult calculator_func_asin(vec_ExprValue args);
static ExprValueResult calculator_func_atan(vec_ExprValue args);
static ExprValueResult calculator_func_sqrt(vec_ExprValue args);
static ExprValueResult calculator_func_ln(vec_ExprValue args);
static ExprValueResult calculator_func_log(vec_ExprValue args);
static ExprValueResult calculator_func_join(vec_ExprValue args);
static ExprValueResult calculator_func_slice(vec_ExprValue args);
static ExprValueResult calculator_func_min(vec_ExprValue args);
static ExprValueResult calculator_func_max(vec_ExprValue args);

#define FUNCTIONS                                                         \
  {                                                                       \
    calculator_func_cos, calculator_func_sin, calculator_func_tan,        \
        calculator_func_acos, calculator_func_asin, calculator_func_atan, \
        calculator_func_sqrt, calculator_func_ln, calculator_func_log,    \
        calculator_func_join, calculator_func_slice, calculator_func_min, \
        calculator_func_max,                                              \
  }

NativeFnPtr calculator_get_native_function(StrSlice name) {
  const char* const names[] = NATIVE_FUNCTION_NAMES;
  NativeFnPtr const functions[] = FUNCTIONS;

  assert_m(LEN(names) == LEN(functions));
  for (int i = 0; i < (int)LEN(names); i++) {
    if (str_slice_eq_ccp(name, names[i])) return functions[i];
  }

  return null;
}

static double basic_cos(double a) { return cos(a); }
static double basic_sin(double a) { return sin(a); }
static double basic_tan(double a) { return tan(a); }
static double basic_acos(double a) { return acos(a); }
static double basic_asin(double a) { return asin(a); }
static double basic_atan(double a) { return atan(a); }
static double basic_sqrt(double a) { return sqrt(a); }
static double basic_ln(double a) { return log(a); }
static double basic_log(double a) { return log(a) / log(10.0); }

static vec_ExprValue template_unary_function_base(vec_ExprValue args,
                                                  double (*fn)(double)) {
  assert_m(fn);

  for (int arg = 0; arg < args.length; arg++) {
    ExprValue* item = &args.data[arg];

    if (item->type is EXPR_VALUE_NONE) {
      // nothing to do
    } else if (item->type is EXPR_VALUE_NUMBER) {
      item->number = fn(item->number);
    } else if (item->type is EXPR_VALUE_VEC) {
      item->vec = template_unary_function_base(item->vec, fn);
    } else {
      panic("Unknown ExprValue type");
    }
  }

  return args;
}

static ExprValueResult template_unary_function(vec_ExprValue args,
                                               double (*fn)(double)) {
  vec_ExprValue values = template_unary_function_base(args, fn);
  ExprValue result;
  if (values.length is 0) {
    result = (ExprValue){.type = EXPR_VALUE_NONE};
    vec_ExprValue_free(values);
  } else if (values.length is 1) {
    result = vec_ExprValue_popget(&values);
    vec_ExprValue_free(values);
  } else {
    result = (ExprValue){.type = EXPR_VALUE_VEC, .vec = values};
  }

  return (ExprValueResult){
      .is_ok = true,
      .ok = result,
  };
}

ExprValueResult calculator_func_cos(vec_ExprValue args) {
  return template_unary_function(args, basic_cos);
}
ExprValueResult calculator_func_sin(vec_ExprValue args) {
  return template_unary_function(args, basic_sin);
}
ExprValueResult calculator_func_tan(vec_ExprValue args) {
  return template_unary_function(args, basic_tan);
}
ExprValueResult calculator_func_acos(vec_ExprValue args) {
  return template_unary_function(args, basic_acos);
}
ExprValueResult calculator_func_asin(vec_ExprValue args) {
  return template_unary_function(args, basic_asin);
}
ExprValueResult calculator_func_atan(vec_ExprValue args) {
  return template_unary_function(args, basic_atan);
}
ExprValueResult calculator_func_sqrt(vec_ExprValue args) {
  return template_unary_function(args, basic_sqrt);
}
ExprValueResult calculator_func_ln(vec_ExprValue args) {
  return template_unary_function(args, basic_ln);
}
ExprValueResult calculator_func_log(vec_ExprValue args) {
  return template_unary_function(args, basic_log);
}

ExprValueResult calculator_func_join(vec_ExprValue args) {
  vec_ExprValue values = vec_ExprValue_create();

  for (int i = 0; i < args.length; i++) {
    ExprValue arg = args.data[i];
    if (arg.type is EXPR_VALUE_VEC) {
      for (int j = 0; j < arg.vec.length; j++) {
        vec_ExprValue_push(&values, arg.vec.data[j]);
      }
      arg.vec.length = 0;
      expr_value_free(arg);
    } else {
      vec_ExprValue_push(&values, arg);
    }
  }
  args.length = 0;  // All elements extracted
  vec_ExprValue_free(args);

  return (ExprValueResult){.is_ok = true,
                           .ok = {.type = EXPR_VALUE_VEC, .vec = values}};
}

#define Err(text) \
  (ExprValueResult) { .is_ok = false, .err_pos = null, .err_text = (text) }
#define EPSILON 0.0001

static long long check_num_integer(double number, ExprValueResult* res) {
  int result = 0;
  if (fabs(round(number) - number) > EPSILON) {
    (*res) = Err(
        str_owned("Slice error: number %lf is not an integer (error of +-" STR(
                      EPSILON) " from integer value is allowed)",
                  number));
  } else {
    result = (long)round(number);
  }
  return result;
}

static ExprValueResult slice_base(vec_ExprValue indices, vec_ExprValue* data);
static ExprValueResult slice_process_index(ExprValue index,
                                           vec_ExprValue* data) {
  ExprValueResult result = {.is_ok = true};
  if (index.type is EXPR_VALUE_NONE) {
    // none index -> none data
    result.ok = (ExprValue){.type = EXPR_VALUE_NONE};
  } else if (index.type is EXPR_VALUE_NUMBER) {
    long long number = check_num_integer(index.number, &result);

    if (result.is_ok) {
      if (number >= 0 and number < (long long)data->length) {
        result.ok = expr_value_clone(&data->data[number]);
      } else {
        result =
            Err(str_owned("Index %lld is out of bounds for vector of size %d",
                          number, data->length));
      }
    }
  } else if (index.type is EXPR_VALUE_VEC) {
    result = slice_base(index.vec, data);
    index.type = EXPR_VALUE_NONE;
    // no need to free
  } else {
    panic("Unknown ExprValue type");
  }
  expr_value_free(index);

  return result;
}

static ExprValueResult slice_base(vec_ExprValue indices, vec_ExprValue* data) {
  ExprValueResult res = {.is_ok = true};
  for (int i = 0; i < indices.length and res.is_ok; i++) {
    res = slice_process_index(indices.data[i], data);

    if (res.is_ok)
      indices.data[i] = res.ok;
    else
      indices.data[i].type = EXPR_VALUE_NONE;
  }

  if (res.is_ok)
    res.ok = (ExprValue){.type = EXPR_VALUE_VEC, .vec = indices};
  else
    vec_ExprValue_free(indices);

  return res;
}

ExprValueResult calculator_func_slice(vec_ExprValue args) {
  ExprValueResult result = {.is_ok = true};
  if (args.length != 2) {
    result = Err(str_literal(
        "Function 'slice' requires two args - data vector and indices vector"));
  } else if (args.data[0].type != EXPR_VALUE_VEC) {
    result =
        Err(str_literal("Function 'slice': first argument is not a vector"));
  } else if (args.data[1].type != EXPR_VALUE_VEC) {
    result =
        Err(str_literal("Function 'slice': second argument is not a vector"));
  } else {
    vec_ExprValue indices = vec_ExprValue_popget(&args).vec;
    vec_ExprValue data = vec_ExprValue_popget(&args).vec;
    vec_ExprValue_free(args);
    result = slice_base(indices, &data);
    vec_ExprValue_free(data);
  }
  return result;
}

void push_min_value(double val, double* min, bool* is_init) {
  if (*is_init)
    (*min) = (*min) < val ? (*min) : val;
  else {
    (*min) = val;
    (*is_init) = true;
  }
}

ExprValueResult calculator_func_min(vec_ExprValue args) {
  double min = 0.0;
  bool has_value = false;

  for (int i = 0; i < args.length; i++) {
    ExprValue arg = args.data[i];

    if (arg.type is EXPR_VALUE_NONE) {
      // skip
    } else if (arg.type is EXPR_VALUE_NUMBER) {
      push_min_value(arg.number, &min, &has_value);
    } else if (arg.type is EXPR_VALUE_VEC) {
      ExprValueResult res = calculator_func_min(arg.vec);
      assert_m(res.is_ok);
      ExprValue value = res.ok;
      if (value.type is EXPR_VALUE_NUMBER) {
        push_min_value(value.number, &min, &has_value);
      } else {
        assert_m(value.type is EXPR_VALUE_NONE);
      }
    } else {
      panic("Unknown ExprValue type");
    }
  }

  debugln("Calculated min: %lf %d", min, has_value);
  return (ExprValueResult){
      .is_ok = true,
      .ok = {.type = has_value ? EXPR_VALUE_NUMBER : EXPR_VALUE_NONE,
             .number = min},
  };
}

void push_max_value(double val, double* max, bool* is_init) {
  if (*is_init)
    (*max) = (*max) > val ? (*max) : val;
  else {
    (*max) = val;
    (*is_init) = true;
  }
}

ExprValueResult calculator_func_max(vec_ExprValue args) {
  double max = 0.0;
  bool has_value = false;

  for (int i = 0; i < args.length; i++) {
    ExprValue arg = args.data[i];

    if (arg.type is EXPR_VALUE_NONE) {
      // skip
    } else if (arg.type is EXPR_VALUE_NUMBER) {
      push_max_value(arg.number, &max, &has_value);
    } else if (arg.type is EXPR_VALUE_VEC) {
      ExprValueResult res = calculator_func_max(arg.vec);
      assert_m(res.is_ok);
      ExprValue value = res.ok;
      if (value.type is EXPR_VALUE_NUMBER) {
        push_max_value(value.number, &max, &has_value);
      } else {
        assert_m(value.type is EXPR_VALUE_NONE);
      }
    } else {
      panic("Unknown ExprValue type");
    }
  }

  return (ExprValueResult){
      .is_ok = true,
      .ok = {.type = has_value ? EXPR_VALUE_NUMBER : EXPR_VALUE_NONE,
             .number = max},
  };
}