#include "calc_expr.h"

#include "../util/allocator.h"
#include "../util/common_vecs.h"
#include "../util/prettify_c.h"

#define VECTOR_C CalcExpr
#define VECTOR_ITEM_DESTRUCTOR calc_expr_free
#define VECTOR_ITEM_CLONE calc_expr_clone
#include "../util/vector.h"

void calc_expr_free(CalcExpr this) {
  expr_free(this.expression);

  if (this.type is CALC_EXPR_VARIABLE) {
    str_free(this.variable_name);

  } else if (this.type is CALC_EXPR_PLOT) {
    // nothing
  } else if (this.type is CALC_EXPR_FUNCTION) {
    str_free(this.function.name);
    vec_str_t_free(this.function.args);

  } else if (this.type is CALC_EXPR_ACTION) {
    // nothinh
  } else {
    panic("Unknown CalcExpr type: %d", this.type);
  }
}

CalcExpr calc_expr_clone(const CalcExpr* source) {
  CalcExpr result = {
      .type = source->type,
      .expression = expr_clone(&source->expression),
  };

  if (source->type is CALC_EXPR_VARIABLE) {
    result.variable_name = str_clone(&source->variable_name);
  } else if (source->type is CALC_EXPR_PLOT) {
    // nothing
  } else if (source->type is CALC_EXPR_FUNCTION) {
    result.function.name = str_clone(&source->function.name);
    result.function.args = vec_str_t_clone(&source->function.args);
  } else if (source->type is CALC_EXPR_ACTION) {
    // nothinh
  } else {
    panic("Unknown CalcExpr type: %d", source->type);
  }

  return result;
}
const char* calc_expr_type_text(int type) {
  if (type is CALC_EXPR_VARIABLE) {
    return "Variable";
  } else if (type is CALC_EXPR_PLOT) {
    return "Plot";
  } else if (type is CALC_EXPR_FUNCTION) {
    return "Function";
  } else if (type is CALC_EXPR_ACTION) {
    return "Action";
  } else {
    panic("Unknown CalcExpr type: %d", type);
  }
}

void calc_expr_print(const CalcExpr* this, OutStream stream) {
  x_sprintf(stream, "CalcExpr.%s", calc_expr_type_text(this->type));

  if (this->type is CALC_EXPR_VARIABLE) {
    x_sprintf(stream, "[%s]", this->variable_name.string);
  } else if (this->type is CALC_EXPR_FUNCTION) {
    x_sprintf(stream, "[%s](", this->function.name.string);

    for (int i = 0; i < this->function.args.length; i++) {
      if (i > 0) outstream_puts(", ", stream);
      outstream_puts(this->function.args.data[i].string, stream);
    }
    outstream_puts(")", stream);
  }

  x_sprintf(stream, "(%$expr)", this->expression);
}
