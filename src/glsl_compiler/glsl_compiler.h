#ifndef SRC_CALCULATOR_GLSL_RENDERER_H_
#define SRC_CALCULATOR_GLSL_RENDERER_H_

#include <stdbool.h>

#include "../calculator/calc_backend.h"
#include "../parser/expr.h"
#include "../util/better_string.h"
#include "glsl_context.h"
#include "glsl_function.h"

StrResult glsl_compile_expression(ExprContext calc, GlslContext* glsl,
                                  const Expr* expr, const vec_str_t* used_args);

#endif  // SRC_CALCULATOR_GLSL_RENDERER_H_