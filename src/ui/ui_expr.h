#ifndef SRC_UI_EXPR_H_
#define SRC_UI_EXPR_H_

#include "../nuklear_flags.h"
#include "../util/better_string.h"

typedef struct ui_expr {
  struct nk_text_edit textedit;
  struct nk_colorf color;

  bool prev_active;
  int prev_length;
  struct nk_colorf prev_color;
  const char* prev_buffer;

  str_t descr_text;
} ui_expr_t;
ui_expr_t ui_expr_create(const char* text);
void ui_expr_free(ui_expr_t this);

// vec_ui_expr
typedef ui_expr_t ui_expr;
#define VECTOR_H ui_expr
#include "../util/vector.h"

#endif  // SRC_UI_EXPR_H_