#include <string.h>

#include "better_io.h"
#include "better_string.h"

typedef void (*PrinterFn)(OutStream stream, VaListWrap* list,
                          int* total_written);

static void printer_slice(OutStream stream, VaListWrap* list,
                          int* total_written);
static void printer_token(OutStream stream, VaListWrap* list,
                          int* total_written);
static void printer_token_tree(OutStream stream, VaListWrap* list,
                               int* total_written);
static void printer_expr_value(OutStream stream, VaListWrap* list,
                               int* total_written);
static void printer_printable(OutStream stream, VaListWrap* list,
                              int* total_written);
static void printer_expr(OutStream stream, VaListWrap* list,
                         int* total_written);

static void printer_calc_expr(OutStream stream, VaListWrap* list,
                              int* total_written);
static void printer_calc_value(OutStream stream, VaListWrap* list,
                               int* total_written);

#define FORMATS                                                              \
  {                                                                          \
    "$token_tree", "$calc_value", "$calc_expr", "$expr_value", "$printable", \
        "$slice", "$token", "$expr"                                          \
  }
#define PRINTERS                                                             \
  {                                                                          \
    printer_token_tree, printer_calc_value, printer_calc_expr,               \
        printer_expr_value, printer_printable, printer_slice, printer_token, \
        printer_expr                                                         \
  }

int x_printf_ext_fmt_length(const char* format) {
  if (format[0] is '\0') return 0;

  format++;

  const char* const formats[] = FORMATS;
  for (int i = 0; i < (int)LEN(formats); i++) {
    int len = strlen(formats[i]);
    if (strncmp(format, formats[i], len) is 0) {
      return len + 1;
    }
  }

  return 0;
}
void x_printf_ext_print(OutStream stream, const char* format, VaListWrap* list,
                        int* total_written) {
  if (format[0] is '\0') return;
  format++;

  const char* const formats[] = FORMATS;
  const PrinterFn printers[] = PRINTERS;
  for (int i = 0; i < (int)LEN(formats); i++)
    if (strncmp(format, formats[i], strlen(formats[i])) is 0) {
      printers[i](stream, list, total_written);
      return;
    }

  return;
}

static void printer_slice(OutStream stream, VaListWrap* list,
                          int* total_written) {
  StrSlice slice = va_arg(list->list, StrSlice);
  outstream_put_slice(slice.start, slice.length, stream);
  (*total_written) += slice.length;
}

static void printer_printable(OutStream stream, VaListWrap* list,
                              int* total_written) {
  Printable val = va_arg(list->list, Printable);
  printable_print(val, stream);
  (*total_written) += 5;  // TODO
}

// TOKENS STUFF

#include "../parser/tokenizer.h"

static void printer_token(OutStream stream, VaListWrap* list,
                          int* total_written) {
  Token t = va_arg(list->list, Token);
  token_print(&t, stream);
  (*total_written) += 5;  // TODO: TOTAL WRITTEN FOR TOKENS
}

#include "../parser/token_tree.h"

static void printer_token_tree(OutStream stream, VaListWrap* list,
                               int* total_written) {
  TokenTree t = va_arg(list->list, TokenTree);
  token_tree_print(&t, stream);
  (*total_written) += 5;  // TODO: TOTAL WRITTEN FOR TOKENTREE
}

#include "../parser/expr_value.h"

static void printer_expr_value(OutStream stream, VaListWrap* list,
                               int* total_written) {
  ExprValue val = va_arg(list->list, ExprValue);
  expr_value_print(&val, stream);
  (*total_written) += 5;  // TODO: TOTAL WRITTEN FOR EXPRVALUE
}
#include "../parser/expr.h"

static void printer_expr(OutStream stream, VaListWrap* list,
                         int* total_written) {
  Expr val = va_arg(list->list, Expr);
  expr_print(&val, stream);
  (*total_written) += 5;  // TODO: TOTAL WRITTEN FOR EXPR
}

#include "../calculator/calc_expr.h"

static void printer_calc_expr(OutStream stream, VaListWrap* list,
                              int* total_written) {
  CalcExpr val = va_arg(list->list, CalcExpr);
  calc_expr_print(&val, stream);
  (*total_written) += 5;  // TODO: TOTAL WRITTEN
}

#include "../calculator/calc_value.h"

static void printer_calc_value(OutStream stream, VaListWrap* list,
                               int* total_written) {
  CalcValue val = va_arg(list->list, CalcValue);
  calc_value_print(&val, stream);
  (*total_written) += 5;  // TODO: TOTAL WRITTEN
}