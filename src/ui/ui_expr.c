#include "ui_expr.h"

#include "../util/allocator.h"

ui_expr ui_expr_clone_panic(const ui_expr* this) {
  unused(this);
  panic("ui_expr cloning is not supported!");
}

#define VECTOR_C ui_expr
#define VECTOR_ITEM_DESTRUCTOR ui_expr_free
#define VECTOR_ITEM_CLONE ui_expr_clone_panic
#include "../util/vector.h"

ui_expr_t ui_expr_create(const char* text) {
  ui_expr_t this = {
      .color = {.r = 0.8, .g = 0.2, .b = 0.1, .a = 1.0},
      .prev_active = false,
      .descr_text = str_literal("Faz balls"),
  };

  nk_textedit_init_default(&this.textedit);
  nk_str_insert_text_char(&this.textedit.string, 0, text, strlen(text));

  this.prev_length = nk_str_len(&this.textedit.string);
  this.prev_buffer = nk_str_get_const(&this.textedit.string);

  this.color.r = 0.8f;
  this.color.g = 0.2f;
  this.color.b = 0.1f;
  this.color.a = 1.0f;
  // debugln("Created ui expr from '%s': descr (%d %p '%s') text '%.*s'", text,
  // this.descr_text.is_owned, this.descr_text.string, this.descr_text.string,
  // this.prev_length, this.prev_buffer);
  return this;
}

void ui_expr_free(ui_expr_t this) {
  nk_textedit_free(&this.textedit);
  str_free(this.descr_text);
}
