#ifndef SRC_CALCULATOR_GLSL_RENDERER_H_
#define SRC_CALCULATOR_GLSL_RENDERER_H_

#include <stdbool.h>

#include "../util/better_string.h"
#include "calculator.h"

typedef struct StrResult {
  bool is_ok;
  str_t data;
} StrResult;
void str_result_free(StrResult this);

typedef struct GlslFunction {
  str_t name;
  vec_str_t args;
  str_t code;
} GlslFunction;
void glsl_function_free(GlslFunction this);
str_t glsl_function_to_text(GlslFunction* this);

#define VECTOR_H GlslFunction
#include "../util/vector.h"  // vec_GlslFunction

typedef struct GlslContext {
  vec_GlslFunction functions;
} GlslContext;

GlslContext glsl_context_create();
void glsl_context_free(GlslContext this);

str_t glsl_context_get_unique_fn_name(GlslContext* this);
str_t glsl_context_get_all_functions(GlslContext* this);
void glsl_context_add_function(GlslContext* this, GlslFunction fn);
GlslFunction* glsl_context_get_function(GlslContext* this, const char* fn_name);

StrResult calc_backend_to_glsl_code(CalcBackend* this, GlslContext* glsl,
                                    Expr* expr, vec_str_t* used_args);

#endif  // SRC_CALCULATOR_GLSL_RENDERER_H_