#include "glsl_context.h"

#include "../util/allocator.h"
#include "../util/better_string.h"
#include "../util/prettify_c.h"

GlslContext glsl_context_create() {
  return (GlslContext){
      .functions = vec_GlslFunction_create(),
  };
}

void glsl_context_free(GlslContext this) {
  vec_GlslFunction_free(this.functions);
}

str_t glsl_context_get_unique_fn_name(GlslContext* this) {
  int funcs_count = this->functions.length;
  int attempt = 0;

  while (true) {
    str_t name = str_owned("uniq_%d_%d", funcs_count, attempt);

    if (glsl_context_get_function(this, name.string)) {
      attempt++;
      str_free(name);
    } else {
      return name;
    }
  }
}

void glsl_context_print_all_functions(GlslContext* this, OutStream out) {
  for (int i = 0; i < this->functions.length; i++) {
    if (i > 0) outstream_puts("\n\n", out);

    glsl_function_print(&this->functions.data[i], out);
  }
}

void glsl_context_add_function(GlslContext* this, GlslFunction fn) {
  if (glsl_context_get_function(this, fn.name.string))
    panic("Function %s was already added!", fn.name.string);

  vec_GlslFunction_push(&this->functions, fn);
}

GlslFunction* glsl_context_get_function(GlslContext* this,
                                        const char* fn_name) {
  for (int i = 0; i < this->functions.length; i++)
    if (strcmp(this->functions.data[i].name.string, fn_name) is 0)
      return &this->functions.data[i];

  return null;
}
