#ifndef SRC_GLSL_COMPILER_H_
#define SRC_GLSL_COMPILER_H_

#include "../util/better_io.h"
#include "../util/better_string.h"

typedef struct GlslFunction {
  str_t name;
  vec_str_t args;
  str_t code;
} GlslFunction;
void glsl_function_free(GlslFunction this);
GlslFunction glsl_function_clone(const GlslFunction* this);

void glsl_function_print(const GlslFunction* this, OutStream out);
void glsl_print_args(const vec_str_t* used_args, OutStream out);
str_t glsl_args_to_string(const vec_str_t* used_args);
str_t glsl_args_vals_to_string(const vec_str_t* used_args);

#define VECTOR_H GlslFunction
#include "../util/vector.h"  // vec_GlslFunction

#endif  // SRC_GLSL_COMPILER_H_