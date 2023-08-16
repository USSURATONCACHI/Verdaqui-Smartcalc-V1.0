#include "out_stream.h"

#include <limits.h>
#include <stdio.h>

#include "../better_string/str_t.h"
#include "../prettify_c.h"

int outstream_putc(int c, OutStream this) {
  return this.vtable->putc(this.data, c);
}
int outstream_puts(const char* string, OutStream this) {
  return this.vtable->puts(this.data, string);
}
int outstream_put_slice(const char* string, size_t length, OutStream this) {
  return this.vtable->put_slice(this.data, string, length);
}
size_t outstream_available_space(OutStream this) {
  return this.vtable->get_available_size(this.data);
}
str_t outstream_description(OutStream this) {
  return this.vtable->description(this.data);
}

// void outstream_x_vprintf(OutStream this, va_list list);
// void outstream_x_printf(OutStream this, ...);

static int putc_file(FILE* this, int c) { return putc(c, this); }
static int puts_file(FILE* this, const char* str) { return fputs(str, this); }
static int put_slice_file(FILE* this, const char* str, size_t length) {
  fprintf(this, "%.*s", (int)length, str);
  return '\n';
}
static size_t get_size_vtable(FILE* this) {
  unused(this);
  return SIZE_MAX;
}
static str_t description_file(FILE* this) {
  unused(this);
  return str_literal("FILE*");
}
static str_t description_stout(FILE* this) {
  unused(this);
  return str_literal("stdout");
}
static str_t description_stderr(FILE* this) {
  unused(this);
  return str_literal("stderr");
}

OutStream outstream_from_file(FILE* file) {
  static const OutStreamVtable FILE_VTABLE = {
      .putc = (void*)putc_file,
      .puts = (void*)puts_file,
      .put_slice = (void*)put_slice_file,
      .get_available_size = (void*)get_size_vtable,
      .description = (void*)description_file};

  return (OutStream){.data = file, .vtable = &FILE_VTABLE};
}

OutStream outstream_stdout() {
  static const OutStreamVtable STDOUT_VTABLE = {
      .putc = (void*)putc_file,
      .puts = (void*)puts_file,
      .put_slice = (void*)put_slice_file,
      .get_available_size = (void*)get_size_vtable,
      .description = (void*)description_stout};

  return (OutStream){.data = stdout, .vtable = &STDOUT_VTABLE};
}
OutStream outstream_stderr() {
  static const OutStreamVtable STDERR_VTABLE = {
      .putc = (void*)putc_file,
      .puts = (void*)puts_file,
      .put_slice = (void*)put_slice_file,
      .get_available_size = (void*)get_size_vtable,
      .description = (void*)description_stderr};

  return (OutStream){.data = stderr, .vtable = &STDERR_VTABLE};
}

static int putc_buf(BufferOutStream* this, int c) {
  if (this->pos < this->length) {
    this->buffer[this->pos++] = c;
    return c;
  } else {
    return EOF;
  }
}
static int puts_buf(BufferOutStream* this, const char* str) {
  if (this->pos >= this->length) return EOF;

  size_t i;
  for (i = 0; str[i] != '\0' and (this->pos + i + 1) < this->length; i++)
    this->buffer[i + this->pos] = str[i];
  this->buffer[i + this->pos] = '\0';
  this->pos += i + 1;

  if (str[i] is '\0') {
    return '\n';
  } else {
    return EOF;
  }
}

static int put_slice_buf(BufferOutStream* this, const char* str,
                         size_t length) {
  if (this->pos >= this->length) return EOF;

  size_t i;
  for (i = 0; i < length and (this->pos + i + 1) < this->length; i++)
    this->buffer[i + this->pos] = str[i];
  this->buffer[i + this->pos] = '\0';
  this->pos += i + 1;

  if (str[i] is '\0') {
    return '\n';
  } else {
    return EOF;
  }
}
static size_t get_size_buf(BufferOutStream* this) {
  return this->pos - this->length;
}
static str_t description_buf(BufferOutStream* this) {
  unused(this);
  return str_literal("BufferOutStream(?)");
}

OutStream outstream_from_buffer(BufferOutStream* b) {
  static const OutStreamVtable BUFFER_VTABLE = {
      .putc = (void*)putc_buf,
      .puts = (void*)puts_buf,
      .put_slice = (void*)put_slice_buf,
      .get_available_size = (void*)get_size_buf,
      .description = (void*)description_buf};

  return (OutStream){.data = b, .vtable = &BUFFER_VTABLE};
}