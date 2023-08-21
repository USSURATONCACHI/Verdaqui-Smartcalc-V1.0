#ifndef SRC_UTIL_STR_T_H_
#define SRC_UTIL_STR_T_H_

#include <stdbool.h>

#include "str_t_raw.h"

str_t str_literal(const char* literal);
str_t str_borrow(const str_t* from);
str_t str_owned(const char* format, ...);
str_t str_raw_owned(char* text);
str_t str_clone(const str_t* source);
void str_free(str_t s);
void str_free_p(str_t* s);

str_t read_file_to_str(const char* filename);

typedef struct StrResult {
  bool is_ok;
  str_t data;
} StrResult;
void str_result_free(StrResult this);

#define StrErr(e) \
  (StrResult) { .is_ok = false, .data = e }

#define StrOk(e) \
  (StrResult) { .is_ok = true, .data = e }

#endif  // SRC_UTIL_STR_T_H_