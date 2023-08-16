
#include "../util/allocator.h"
#include "expr.h"
#include "operators_fns.h"

// -- Computation

// =====
// =
// = expr_calculate
// =
// =====

#define OkNum(num)                                                  \
  (ExprValueResult) {                                               \
    .is_ok = true, .ok.type = EXPR_VALUE_NUMBER, .ok.number = (num) \
  }

static ExprValueResult expr_calculate_function(const ExprFunction* this,
                                               ExprContext ctx);
static ExprValueResult expr_calculate_vector(const ExprVector* this,
                                             ExprContext ctx);
static ExprValueResult expr_calculate_binary_op(const ExprBinaryOp* this,
                                                ExprContext ctx);

ExprValueResult expr_calculate(const Expr* this, ExprContext ctx) {
  assert_m(this);
  assert_m(ctx.vtable and ctx.vtable->get_variable_val and
           ctx.vtable->call_function);

  if (this->type is EXPR_NUMBER) {
      return OkNum(this->number.value);

  } else if (this->type is EXPR_VARIABLE) {
    return ctx.vtable->get_variable_val(ctx.data, str_slice_from_str_t(&this->variable.name));

  } else if (this->type is EXPR_FUNCTION) {
      return expr_calculate_function(&this->function, ctx);

  } else if (this->type is EXPR_VECTOR) {
      return expr_calculate_vector(&this->vector, ctx);

  } else if (this->type is EXPR_BINARY_OP) {
      return expr_calculate_binary_op(&this->binary_operator, ctx);

  } else {
      panic("Invalid expr type");

  }
}


static ExprValueResult expr_calculate_function(const ExprFunction* this,
                                               ExprContext ctx) {
  assert_m(this);
  // FUNCTION
  StrSlice function_name = str_slice_from_str_t(&this->name);
  ExprValueResult res = expr_calculate(this->argument, ctx);
  if (not res.is_ok) return res;

  vec_ExprValue args;

  if (res.ok.type is EXPR_VALUE_NUMBER) {
    args = vec_ExprValue_create();
    vec_ExprValue_push(&args, res.ok);

  } else if (res.ok.type is EXPR_VALUE_VEC) {
    args = res.ok.vec;

  } else if (res.ok.type is EXPR_VALUE_NONE) {
    args = vec_ExprValue_create();

  } else {
    panic("Unknown ExprValue type");
  }

  ExprValueResult call_res = ctx.vtable->call_function(ctx.data, function_name, &args);
  vec_ExprValue_free(args);
  return call_res;
}

static ExprValueResult expr_calculate_vector(const ExprVector* this,
                                             ExprContext ctx) {
  assert_m(this);
  // VECTOR
  const vec_Expr* args_expr = &this->arguments;
  vec_ExprValue args = vec_ExprValue_create();

  ExprValueResult res = {.is_ok = true};
  for (int i = 0; i < args_expr->length and res.is_ok; i++) {
    res = expr_calculate(&args_expr->data[i], ctx);

    if (res.is_ok) vec_ExprValue_push(&args, res.ok);
  }

  if (res.is_ok) {
    res = (ExprValueResult){.is_ok = true,
                            .ok = (ExprValue){
                                .type = EXPR_VALUE_VEC,
                                .vec = args,
                            }};
  } else {
    vec_ExprValue_free(args);
  }

  return res;
}

static ExprValueResult expr_calculate_binary_op(const ExprBinaryOp* this,
                                                ExprContext ctx) {
  assert_m(this);
  // BINARY OPERATOR
  ExprValueResult res = expr_calculate(this->lhs, ctx);
  if (res.is_ok) {
    ExprValue a = res.ok;

    res = expr_calculate(this->rhs, ctx);
    if (res.is_ok) {
      ExprValue b = res.ok;
      OperatorFn operator = expr_get_operator_fn(this->name.string);
      assert_m(operator);

      res = operator(a, b);
    } else {
      expr_value_free(a);
    }
  }

  return res;
}
