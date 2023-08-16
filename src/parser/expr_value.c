
#include "expr_value.h"

#include "../util/allocator.h"

#define VECTOR_C ExprValue
#define VECTOR_ITEM_DESTRUCTOR expr_value_free
#define VECTOR_ITEM_CLONE expr_value_clone
#include "../util/vector.h"

// =====
// =
// = expr_value_free
// =
// =====
void expr_value_free(ExprValue this) {
  if (this.type is EXPR_VALUE_NUMBER) {
    // nothing
  } else if (this.type is EXPR_VALUE_VEC) {
    vec_ExprValue_free(this.vec);
  } else if (this.type is EXPR_VALUE_NONE) {
    // nothing
  } else {
    panic("Unknown ExprValue type");
  }
}

// =====
// =
// = expr_value_clone
// =
// =====
ExprValue expr_value_clone(const ExprValue* source) {
  ExprValue res;
  res.type = source->type;

  if (res.type is EXPR_VALUE_NUMBER) {
    res.number = source->number;
  } else if (res.type is EXPR_VALUE_VEC) {
    res.vec = vec_ExprValue_create_copy(source->vec.data, source->vec.length);
  } else if (res.type is EXPR_VALUE_NONE) {
    // do nothing
  } else {
    panic("Unknown ExprValue type");
  }

  return res;
}

// =====
// =
// = expr_value_print
// =
// =====
void expr_value_print(const ExprValue* this, OutStream stream) {
  if (this->type is EXPR_VALUE_NUMBER) {
    x_sprintf(stream, "%.2lf", this->number);

  } else if (this->type is EXPR_VALUE_VEC) {
    outstream_putc('[', stream);
    for (int i = 0; i < this->vec.length; i++) {
      if (i > 0) outstream_puts(", ", stream);

      expr_value_print(&this->vec.data[i], stream);
    }
    outstream_putc(']', stream);

  } else if (this->type is EXPR_VALUE_NONE) {
    outstream_puts("()", stream);

  } else {
    panic("Unknown ExprValue type");
  }
}

// =====
// =
// = expr_value_type_text
// =
// =====
const char* expr_value_type_text(int type) {
  switch (type) {
    case EXPR_VALUE_NONE:
      return "None";
    case EXPR_VALUE_NUMBER:
      return "Number";
    case EXPR_VALUE_VEC:
      return "Vec";
    default:
      panic("Unknown ExprValue type");
  }
}
