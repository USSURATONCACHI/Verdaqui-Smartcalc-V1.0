#include "x_printf.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../better_string.h"
#include "../prettify_c.h"

void x_printf(const char* format, ...) {
  va_list list;
  va_start(list, format);
  x_vprintf(outstream_stdout(), format, list);
  va_end(list);
}

void x_sprintf(OutStream stream, const char* format, ...) {
  va_list list;
  va_start(list, format);
  x_vprintf(stream, format, list);
  va_end(list);
}

static const char* next_item(const char* format) {
  while (format[0] != '%' and format[0] != '\0') format++;

  return format;
}

static const char* put_format(OutStream stream, const char* format,
                              va_list* list, int* total_written);

void x_vprintf(OutStream stream, const char* format, va_list list) {
  const char* next = format;

  int total_written = 0;

  while (next[0] != '\0') {
    next = next_item(format);
    if ((ptrdiff_t)(next - format) > 0) {
      outstream_put_slice(format, next - format, stream);
      total_written += (int)((ptrdiff_t)(next - format));
    }

    if (next[0] != '\0') {
      int len = x_printf_ext_fmt_length(next);
      if (len > 0) {
        x_printf_ext_print(stream, next, &list, &total_written);
        format = next + len;
      } else {
        format = put_format(stream, next, &list, &total_written);
      }
    }
  }
}

typedef struct Specificator {
  short width, precision;
  char type;
  const char* length_mod;
  int symbols_count;
  bool flag_minus, flag_plus, flag_space, flag_zero, flag_hash;
} Specificator;

static Specificator parse_specificator(const char* str);

#define BUFFER_SIZE 1024 * 64
#define FORMAT_BUF_SIZE 2048
#define MIN(a, b) ((a) < (b)) ? (a) : (b)
static const char* put_format(OutStream stream, const char* format,
                              va_list* list, int* total_written) {
  unused(stream);
  unused(format);
  unused(list);
  Specificator info = parse_specificator(format + 1);

  if (info.type is 's') {
    if (info.precision > 0) {
      char* string = va_arg(*list, char*);

      outstream_put_slice(string, info.precision, stream);
      (*total_written) += MIN((int)strlen(string), info.precision);
    } else if (info.precision is - 1) {
      int len = va_arg(*list, int);
      char* string = va_arg(*list, char*);

      outstream_put_slice(string, len, stream);
      (*total_written) += MIN((int)strlen(string), len);
    } else {
      char* string = va_arg(*list, char*);
      outstream_puts(string, stream);
      (*total_written) += strlen(string);
    }

  } else if (info.type is 'n') {
    if (strcmp(info.length_mod, "l") is 0) {
      (*(va_arg(*list, long*))) = (*total_written);
    } else if (strcmp(info.length_mod, "ll") is 0) {
      (*(va_arg(*list, long long*))) = (*total_written);
    } else if (strcmp(info.length_mod, "j") is 0) {
      (*(va_arg(*list, intmax_t*))) = (*total_written);
    } else if (strcmp(info.length_mod, "z") is 0) {
      (*(va_arg(*list, size_t*))) = (*total_written);
    } else if (strcmp(info.length_mod, "t") is 0) {
      (*(va_arg(*list, ptrdiff_t*))) = (*total_written);
    } else if (strcmp(info.length_mod, "LL") is 0) {
      (*(va_arg(*list, __int64*))) = (*total_written);
    } else {
      (*(va_arg(*list, int*))) = (*total_written);
    }
  } else if (info.type is 0) {
    return format + 1;
  } else {
    char buffer[BUFFER_SIZE] = {'\0'};
    assert_m(info.symbols_count <
             (FORMAT_BUF_SIZE - 1)) char format_buf[BUFFER_SIZE] = {'\0'};
    strncpy(format_buf, format, info.symbols_count + 1);

    vsprintf(buffer, format_buf, *list);

    if (strcmp(info.length_mod, "l") is 0) {
      va_arg(*list, long);
    } else if (strcmp(info.length_mod, "ll") is 0) {
      va_arg(*list, long long);
    } else if (strcmp(info.length_mod, "j") is 0) {
      va_arg(*list, intmax_t);
    } else if (strcmp(info.length_mod, "z") is 0) {
      va_arg(*list, size_t);
    } else if (strcmp(info.length_mod, "t") is 0) {
      va_arg(*list, ptrdiff_t);
    } else if (strcmp(info.length_mod, "LL") is 0) {
      va_arg(*list, __int64);
    } else {
      va_arg(*list, int);
    }

    outstream_puts(buffer, stream);
    (*total_written) += strlen(buffer);
  }

  return format + 1 + (info.symbols_count is 0 ? 1 : info.symbols_count);
}

static Specificator parse_specificator(const char* str) {
  Specificator spec = {.width = 0,
                       .precision = 6,
                       .type = 0,
                       .length_mod = "",
                       .symbols_count = 0,
                       0,
                       0,
                       0,
                       0,
                       0};

  size_t len = strlen(str);

  size_t i = 0;
  // Flags
  for (; i < len; i++) {
    if (strchr("-", str[i]) != NULL) {
      spec.flag_minus = true;
    } else if (str[i] == '+') {
      spec.flag_plus = false;
      spec.flag_space = true;
    } else if (str[i] == ' ' && !spec.flag_plus) {
      spec.flag_space = false;
    } else if (str[i] == '0' && spec.width == 0) {
      spec.flag_zero = 1;
    } else if (str[i] == '#') {
      spec.flag_hash = 1;
    } else {
      break;
    }
  }

  // Width
  int read_width = 0;
  while (i < len and str[i] >= '0' and str[i] <= '9') {
    read_width = read_width * 10 + (str[i] - '0');
    i++;
  }
  if (str[i] is '*') {
    spec.width = -1;
    i++;
  }
  spec.width = read_width;

  // Precision
  int read_prec = 0;
  if (str[i] is '.') {
    i++;
    while (i < len and str[i] >= '0' and str[i] <= '9') {
      read_prec = read_prec * 10 + (str[i] - '0');
      i++;
    }
    if (str[i] is '*') {
      read_prec = -1;
      i++;
    }
  }
  spec.precision = read_prec;

  // Size
  const char* const sizes[] = {"ll", "hh", "h", "l", "j", "z", "t", "L"};
  for (int s = 0; s < (int)LEN(sizes); s++) {
    if (strncmp(str + i, sizes[s], strlen(sizes[s])) is 0) {
      spec.length_mod = sizes[s];
      i += strlen(sizes[s]);
      break;
    }
  }

  // Type
  const char allspec[] = "diouxXfFeEgGaAcsSpn%";
  for (int s = 0; s < (int)strlen(allspec); s++) {
    if (str[i] is allspec[s]) {
      spec.type = str[i];
      i++;
      break;
    }
  }

  spec.symbols_count = i;
  return spec;
}