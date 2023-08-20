#include "str_t.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../allocator.h"
#include "../prettify_c.h"
#include "string_stream.h"

str_t str_literal(const char* literal) { return (str_t){literal, false}; }
str_t str_owned(const char* format, ...) {
  va_list list;
  va_start(list, format);

  StringStream builder = string_stream_create();
  OutStream os = string_stream_stream(&builder);
  x_vprintf(os, format, list);
  str_t result = string_stream_to_str_t(builder);

  va_end(list);

  return result;
}

str_t str_borrow(const str_t* from) {
  return (str_t){.is_owned = false, .string = from->string};
}

str_t str_raw_owned(char* text) {
  return (str_t){.is_owned = true, .string = text};
}

void str_free(str_t s) {
  if (s.is_owned and s.string is_not null) {
    // debugln("Freeing str_t at %p (%s)", s.string, s.string);
    FREE((void*)s.string);
  }
}

void str_free_p(str_t* s) {
  if (s is null) return;

  if (s->is_owned and s->string is_not null) FREE((void*)s->string);

  FREE(s);
}

str_t str_clone(const str_t* source) {
  if (source->is_owned) {
    int len = strlen(source->string) + 1;
    char* data = (char*)MALLOC(len * sizeof(char));
    assert_alloc(data);
    strcpy(data, source->string);
    return (str_t){
        .is_owned = source->is_owned,
        .string = data,
    };
  } else {
    return *source;
  }
}

void str_result_free(StrResult this) { str_free(this.data); }

#include "vec_str_t.h"

#define VECTOR_C str_t
#define VECTOR_ITEM_DESTRUCTOR str_free
#define VECTOR_ITEM_CLONE str_clone
#include "../vector.h"