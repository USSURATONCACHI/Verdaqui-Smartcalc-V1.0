#include "str_slice.h"

#include <stdbool.h>
#include <string.h>

#include "../allocator.h"
#include "../prettify_c.h"

bool str_slice_eq_ccp(StrSlice slice, const char* string) {
  return slice.length == (int)strlen(string) and
         strncmp(slice.start, string, slice.length) is 0;
}

bool str_slice_eq(StrSlice a, StrSlice b) {
  return a.length == b.length and strncmp(a.start, b.start, a.length) is 0;
}

str_t str_slice_to_owned(StrSlice slice) {
  char* buffer = (char*)MALLOC((slice.length + 1) * sizeof(char));
  strncpy(buffer, slice.start, slice.length + 1);
  buffer[slice.length] = '\0';
  // debugln("From slice '%.*s' got string '%s'", slice.length, slice.start,
  // buffer);
  return (str_t){.is_owned = true, .string = buffer};
}

StrSlice str_slice_from_str_t(const str_t* str) {
  return (StrSlice){.start = str->string, .length = strlen(str->string)};
}

StrSlice str_slice_from_string(const char* str) {
  return (StrSlice){.start = str, .length = strlen(str)};
}