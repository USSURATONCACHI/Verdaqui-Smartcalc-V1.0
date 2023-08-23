#ifndef SRC_UTIL_OUT_STREAM_H_
#define SRC_UTIL_OUT_STREAM_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "../better_string/str_t_raw.h"

typedef struct OutStreamVtable {
  int (*putc)(void* this, int);
  int (*puts)(void* this, const char* str);
  int (*put_slice)(void* this, const char* str, size_t length);

  size_t (*get_available_size)(void* this);
  str_t (*description)(void* this);
} OutStreamVtable;

typedef struct OutStream {
  void* data;
  const OutStreamVtable* vtable;
} OutStream;

// void outstream_vprintf(OutStream this, va_list list);
// void outstream_printf(OutStream this, ...);
int outstream_putc(int c, OutStream this);
int outstream_puts(const char* string, OutStream this);
int outstream_put_slice(const char* string, size_t length, OutStream this);
size_t outstream_available_space(OutStream this);
str_t outstream_description(OutStream this);

void outstream_x_vprintf(OutStream this, va_list list);
void outstream_x_printf(OutStream this, ...);

OutStream outstream_from_file(FILE* file);
OutStream outstream_stdout();
OutStream outstream_stderr();

typedef struct BufferOutStream {
  char* buffer;
  size_t length;
  size_t pos;
} BufferOutStream;
OutStream outstream_from_buffer(BufferOutStream* b);

#endif  // SRC_UTIL_OUT_STREAM_H_