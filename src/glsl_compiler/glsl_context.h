#ifndef SRC_GLSL_COMPILER_GLSL_CONTEXT_H_
#define SRC_GLSL_COMPILER_GLSL_CONTEXT_H_

#include "glsl_function.h"

typedef struct GlslContext {
  vec_GlslFunction functions;
} GlslContext;

GlslContext glsl_context_create();
void glsl_context_free(GlslContext this);

str_t glsl_context_get_unique_fn_name(GlslContext* this);
void glsl_context_print_all_functions(GlslContext* this, OutStream out);
void glsl_context_add_function(GlslContext* this, GlslFunction fn);
GlslFunction* glsl_context_get_function(GlslContext* this, const char* fn_name);

#endif  // SRC_GLSL_COMPILER_GLSL_CONTEXT_H_