#ifndef SRC_UTIL_PRINTABLE_H_
#define SRC_UTIL_PRINTABLE_H_

#include "out_stream.h"

typedef struct Printable {
  const void* data;
  void (*print)(const void* data, OutStream stream);
} Printable;

void printable_print(Printable this, OutStream out);
void printable_print_std(Printable this);

Printable printable_string(const char* string);
Printable printable_outstream(const OutStream* this);

#endif  // SRC_UTIL_PRINTABLE_H_