#ifndef SRC_PARSER_STR_SLICE_H_
#define SRC_PARSER_STR_SLICE_H_

#include "str_t.h"

typedef struct StrSlice {
  const char* start;
  int length;
} StrSlice;

bool str_slice_eq_ccp(StrSlice slice, const char* string);
bool str_slice_eq(StrSlice a, StrSlice b);
str_t str_slice_to_owned(StrSlice slice);
StrSlice str_slice_from_string(const char* str);
StrSlice str_slice_from_str_t(const str_t* str);

#endif  // SRC_PARSER_STR_SLICE_H_