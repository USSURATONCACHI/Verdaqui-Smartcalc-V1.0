#include "printable.h"

#include "../better_string/str_t.h"
#include "../prettify_c.h"

void printable_print(Printable this, OutStream out) {
  this.print(this.data, out);
}

void printable_print_std(Printable this) {
  this.print(this.data, outstream_stdout());
}

Printable printable_string(const char* string) {
  return (Printable){.data = string, .print = (void*)outstream_puts};
}

static void print_outstream(const OutStream* this, OutStream out) {
  outstream_puts("OutStream(", out);
  str_t descr = outstream_description(*this);
  outstream_puts(descr.string, out);
  str_free(descr);
  outstream_puts(")", out);
}

Printable printable_outstream(const OutStream* this) {
  return (Printable){.data = this, .print = (void*)print_outstream};
}