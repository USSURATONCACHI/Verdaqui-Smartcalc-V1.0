#ifndef SRC_CALCULATOR_FUNC_CONST_CTX_H_
#define SRC_CALCULATOR_FUNC_CONST_CTX_H_

#include "../parser/expr.h"
#include "../util/better_string.h"

typedef struct FuncConstCtx {
  ExprContext parent;
  vec_str_t* used_args;
} FuncConstCtx;

ExprContext func_const_ctx_context(FuncConstCtx* this);

#endif  // SRC_CALCULATOR_FUNC_CONST_CTX_H_