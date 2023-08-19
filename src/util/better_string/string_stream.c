#include "string_stream.h"

#include <string.h>

#include "../allocator.h"
#include "../prettify_c.h"
#include "str_t.h"

#define BUFFER_EXTRA_CAP 128
#define CHAR_REALLOC_COEF 4 / 3

void string_stream_free(StringStream this) { free(this.buffer); }

StringStream string_stream_create() {
  return (StringStream){
      .buffer = null,
      .capacity = 0,
      .length = 0,
  };
}

StringStream string_stream_clone(const StringStream* source) {
  if (source->buffer) {
    size_t new_cap = source->length + BUFFER_EXTRA_CAP;
    char* buffer = (char*)MALLOC(new_cap * sizeof(char));
    assert_alloc(buffer);
    memcpy(buffer, source->buffer, source->length);
    return (StringStream){
        .buffer = buffer,
        .capacity = new_cap,
        .length = source->length,
    };
  } else {
    return (*source);  // No data in hold, can just copy by value
  }
}

void string_stream_print(const StringStream* tthis, OutStream out) {
  outstream_puts("StringStream(", out);
  outstream_put_slice(tthis->buffer, tthis->length, out);
  outstream_puts(")", out);
}

static void ss_realloc(StringStream* this, size_t add_size) {
  size_t new_cap =
      this->capacity * CHAR_REALLOC_COEF + add_size + BUFFER_EXTRA_CAP;
  this->buffer = (char*)REALLOC(this->buffer, new_cap);
  assert_alloc(this->buffer);
  this->capacity = new_cap;
}

static int ss_putc(StringStream* this, int c) {
  if (this->length >= this->capacity) ss_realloc(this, 1);

  this->buffer[this->length++] = c;
  return c;
}

static int ss_puts(StringStream* this, const char* str) {
  size_t len = strlen(str);
  if ((this->length + len) > this->capacity) ss_realloc(this, len);

  memcpy(&this->buffer[this->length], str, sizeof(char) * len);
  this->length += len;
  return '\n';
}

static int ss_put_slice(StringStream* this, const char* str, int length) {
  if (length is 0) return '\n';
  if ((this->length + length) > this->capacity) ss_realloc(this, length);

  memcpy(&this->buffer[this->length], str, sizeof(char) * length);
  this->length += length;
  return '\n';
}

static size_t ss_get_available_size(StringStream* this) {
  unused(this);
  return SIZE_MAX;
}
static str_t ss_description(StringStream* this) {
  unused(this);
  return str_literal("StringStream");
}

OutStream string_stream_stream(StringStream* tthis) {
  static const OutStreamVtable TABLE = {
      .putc = (void*)ss_putc,
      .puts = (void*)ss_puts,
      .put_slice = (void*)ss_put_slice,
      .description = (void*)ss_description,
      .get_available_size = (void*)ss_get_available_size,
  };
  return (OutStream){.data = tthis, .vtable = &TABLE};
}

char* string_stream_collect(StringStream tthis) {
  ss_putc(&tthis, '\0');
  return tthis.buffer;
}

str_t string_stream_to_str_t(StringStream tthis) {
  return str_raw_owned(string_stream_collect(tthis));
}