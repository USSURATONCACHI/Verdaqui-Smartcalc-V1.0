#ifndef SRC_CALCULATOR_NATIVE_FUNCTIONS_H_
#define SRC_CALCULATOR_NATIVE_FUNCTIONS_H_

#include "../parser/expr_value.h"

#define NATIVE_FUNCTION_NAMES                                                 \
  {                                                                           \
    "cos", "sin", "tan", "acos", "asin", "atan", "sqrt", "ln", "log", "join", \
        "slice", "min", "max",                                                \
  }

typedef ExprValueResult (*NativeFnPtr)(vec_ExprValue);

NativeFnPtr calculator_get_native_function(StrSlice name);
/*
ExprValueResult calculator_func_cos(vec_ExprValue args);
ExprValueResult calculator_func_sin(vec_ExprValue args);
ExprValueResult calculator_func_tan(vec_ExprValue args);
ExprValueResult calculator_func_acos(vec_ExprValue args);
ExprValueResult calculator_func_asin(vec_ExprValue args);
ExprValueResult calculator_func_atan(vec_ExprValue args);
ExprValueResult calculator_func_sqrt(vec_ExprValue args);
ExprValueResult calculator_func_ln(vec_ExprValue args);
ExprValueResult calculator_func_log(vec_ExprValue args);
ExprValueResult calculator_func_join(vec_ExprValue args);
ExprValueResult calculator_func_slice(vec_ExprValue args);
ExprValueResult calculator_func_min(vec_ExprValue args);
ExprValueResult calculator_func_max(vec_ExprValue args);
*/

#endif  // SRC_CALCULATOR_NATIVE_FUNCTIONS_H_