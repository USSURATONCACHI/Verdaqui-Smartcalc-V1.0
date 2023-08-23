#ifndef SRC_UTIL_X_PRINTF_H_
#define SRC_UTIL_X_PRINTF_H_

#include <stdarg.h>

#include "out_stream.h"

typedef struct VaListWrap {
  va_list list;
} VaListWrap;

int x_printf_ext_fmt_length(const char* format);
void x_printf_ext_print(OutStream stream, const char* format, VaListWrap* list,
                        int* total_written);

void x_printf(const char* format, ...);
void x_sprintf(OutStream stream, const char* format, ...);
void x_vprintf(OutStream stream, const char* format, VaListWrap list);

#endif  // SRC_UTIL_X_PRINTF_H_