#include "glsl_function.h"

#include "../util/allocator.h"

#define VECTOR_C GlslFunction
#define VECTOR_ITEM_DESTRUCTOR glsl_function_free
#define VECTOR_ITEM_CLONE glsl_function_clone
#include "../util/vector.h"  // vec_GlslFunction

void glsl_function_free(GlslFunction this) {
  str_free(this.name);
  str_free(this.code);
  vec_str_t_free(this.args);
}

GlslFunction glsl_function_clone(const GlslFunction* this) {
  GlslFunction clone = {
      .args = vec_str_t_clone(&this->args),
      .code = str_clone(&this->code),
      .name = str_clone(&this->name),
  };
  return clone;
}

void glsl_function_print(const GlslFunction* this, OutStream out) {
  x_sprintf(out, "float %s(vec2 pos, vec2 step", this->name.string);
  glsl_print_args(&this->args, out);
  x_sprintf(out, "){\n%s\n}", this->code.string);
}

void glsl_print_args(const vec_str_t* used_args, OutStream out) {
  for (int i = 0; i < used_args->length; i++)
    x_sprintf(out, ", float arg_%s", used_args->data[i].string);
}

str_t glsl_args_to_string(const vec_str_t* used_args) {
  StringStream stream = string_stream_create();
  OutStream os = string_stream_stream(&stream);
  glsl_print_args(used_args, os);
  return string_stream_to_str_t(stream);
}

void glsl_print_args_vals(const vec_str_t* used_args, OutStream out) {
  for (int i = 0; i < used_args->length; i++)
    x_sprintf(out, ", arg_%s", used_args->data[i].string);
}

str_t glsl_args_vals_to_string(const vec_str_t* used_args) {
  StringStream stream = string_stream_create();
  OutStream os = string_stream_stream(&stream);
  glsl_print_args_vals(used_args, os);
  return string_stream_to_str_t(stream);
}