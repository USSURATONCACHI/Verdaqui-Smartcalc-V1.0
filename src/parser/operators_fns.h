#ifndef SRC_PARSER_OPERATORS_FNS_H_
#define SRC_PARSER_OPERATORS_FNS_H_

#include "parser.h"

typedef ExprValueResult (*OperatorFn)(ExprValue, ExprValue);
OperatorFn expr_get_operator_fn(const char* name);
OperatorFn expr_get_operator_fn_slice(StrSlice name);

ExprValueResult expr_operator_add(ExprValue, ExprValue);
ExprValueResult expr_operator_sub(ExprValue, ExprValue);
ExprValueResult expr_operator_mul(ExprValue, ExprValue);
ExprValueResult expr_operator_div(ExprValue, ExprValue);
ExprValueResult expr_operator_mod(ExprValue, ExprValue);
ExprValueResult expr_operator_pow(ExprValue, ExprValue);

ExprValueResult expr_operator_index(ExprValue, ExprValue);

ExprValueResult expr_operator_range(ExprValue, ExprValue);
ExprValueResult expr_operator_range_included(ExprValue, ExprValue);

ExprValueResult expr_operator_assign(ExprValue, ExprValue);
ExprValueResult expr_operator_add_assign(ExprValue, ExprValue);
ExprValueResult expr_operator_sub_assign(ExprValue, ExprValue);
ExprValueResult expr_operator_mul_assign(ExprValue, ExprValue);
ExprValueResult expr_operator_div_assign(ExprValue, ExprValue);
ExprValueResult expr_operator_mod_assign(ExprValue, ExprValue);
ExprValueResult expr_operator_pow_assign(ExprValue, ExprValue);

ExprValueResult expr_operator_eq(ExprValue, ExprValue);
ExprValueResult expr_operator_neq(ExprValue, ExprValue);
ExprValueResult expr_operator_lte(ExprValue, ExprValue);
ExprValueResult expr_operator_gte(ExprValue, ExprValue);
ExprValueResult expr_operator_lt(ExprValue, ExprValue);
ExprValueResult expr_operator_gt(ExprValue, ExprValue);

#endif  // SRC_PARSER_OPERATORS_FNS_H_