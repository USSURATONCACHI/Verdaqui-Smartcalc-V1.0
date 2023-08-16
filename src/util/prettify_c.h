#ifndef SRC_PRETTIFY_C_H_
#define SRC_PRETTIFY_C_H_

#include <stddef.h>
#include <stdlib.h>

#include "better_io.h"

#define and &&
#define or ||
#define not !
#define is ==
#define is_not !=
#define isnot !=
#define null NULL

#define unreachable() __builtin_unreachable()

#define panic(...)                                                          \
  {                                                                         \
    OutStream err_stream = outstream_stderr();                              \
    x_sprintf(err_stream, "\n+-+-+-+-+-+-+-+-+-+-\n");                      \
    x_sprintf(err_stream, "Panic in file %s:%d, function %s: \n", __FILE__, \
              __LINE__, __func__);                                          \
    x_sprintf(err_stream, __VA_ARGS__);                                     \
    x_sprintf(err_stream, "\n");                                            \
    exit(-1);                                                               \
  }

#define forget(value)
#define unused(var) var = var

#define DEBUG_OUT outstream_stdout()
#define debugln(...)                                           \
  {                                                            \
    x_sprintf(DEBUG_OUT, "LOG (%s:%d): ", __FILE__, __LINE__); \
    x_sprintf(DEBUG_OUT, __VA_ARGS__);                         \
    x_sprintf(DEBUG_OUT, "\n");                                \
  }
#define debug(...)                                             \
  {                                                            \
    x_sprintf(DEBUG_OUT, "LOG (%s:%d): ", __FILE__, __LINE__); \
    x_sprintf(DEBUG_OUT, __VA_ARGS__);                         \
  }
#define debugc(...) \
  { x_sprintf(DEBUG_OUT, __VA_ARGS__); }

#define assert_alloc(ptr) \
  if (ptr is null) panic("Failed to allocate memory");

#define assert_m(cond) \
  if (not(cond)) panic("Assertion failed\n");

#define CONCAT2(s1, s2) s1##s2
#define CONCAT(s1, s2) CONCAT2(s1, s2)

#define STR2(s) #s
#define STR(s) STR2(s)

#define LEN(array) (sizeof(array) / sizeof(array[0]))

#define SWAP(type, a, b) \
  {                      \
    type tmp = (a);      \
    (a) = (b);           \
    (b) = tmp;           \
  }

#define foreach_extract(item, vec, condition, code)                           \
  {                                                                           \
    int i;                                                                    \
    for (i = 0; i < (vec).length and (condition); i++) {                      \
      item = (vec).data[i];                                                   \
      code;                                                                   \
    }                                                                         \
                                                                              \
    (vec).length -= i;                                                        \
    for (int j = 0; j < (vec).length; j++) (vec).data[j] = (vec).data[j + i]; \
  }

#endif  // SRC_PRETTIFY_C_H_