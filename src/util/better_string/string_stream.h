#ifndef BETTER_STRING_STRING_STREAM_H_
#define BETTER_STRING_STRING_STREAM_H_

#include <stdint.h>

#include "../better_io.h"

typedef struct StringStream {
  char* buffer;
  size_t capacity;
  size_t length;
} StringStream;

void string_stream_free(StringStream tthis);
StringStream string_stream_create();
StringStream string_stream_clone(const StringStream* source);
void string_stream_print(const StringStream* tthis, OutStream out);

OutStream string_stream_stream(StringStream* tthis);
char* string_stream_collect(StringStream tthis);
str_t string_stream_to_str_t(StringStream tthis);

#endif  // BETTER_STRING_STRING_STREAM_H_