#ifndef SRC_PARSER_EXPR_VALUE_H_
#define SRC_PARSER_EXPR_VALUE_H_

#include "../util/better_io.h"
#include "../util/better_string.h"

typedef struct ExprValue ExprValue;

// ===== vec_ExprValue
#define VECTOR_H ExprValue
#include "../util/vector.h"

#define EXPR_VALUE_NUMBER 0
#define EXPR_VALUE_VEC 1
#define EXPR_VALUE_NONE 3
struct ExprValue {
  int type;
  union {
    double number;
    vec_ExprValue vec;
  };
};

void expr_value_free(ExprValue this);
ExprValue expr_value_clone(const ExprValue* source);
void expr_value_print(const ExprValue* this, OutStream stream);
const char* expr_value_type_text(int type);

typedef struct ExprValueResult {
  bool is_ok;

  union {
    ExprValue ok;
    struct {
      str_t err_text;
      const char* err_pos;
    };
  };
} ExprValueResult;

#endif  // SRC_PARSER_EXPR_VALUE_H_