#ifndef SRC_CALCULATOR_CALC_VALUE_H_
#define SRC_CALCULATOR_CALC_VALUE_H_

#include "../parser/expr_value.h"
#include "../util/better_string.h"
#include "../util/better_io.h"

typedef struct CalcValue {
  str_t name;
  ExprValue value;
} CalcValue;

void calc_value_free(CalcValue);
CalcValue calc_value_clone(const CalcValue*);
void calc_value_print(const CalcValue* this, OutStream stream);

#define VECTOR_H CalcValue
#include "../util/vector.h"  // vec_CalcValue

#endif // SRC_CALCULATOR_CALC_VALUE_H_