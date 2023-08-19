#include "calc_value.h"

#include "../util/allocator.h"
#include "../util/prettify_c.h"

#define VECTOR_C CalcValue
#define VECTOR_ITEM_DESTRUCTOR calc_value_free
#define VECTOR_ITEM_CLONE calc_value_clone
#include "../util/vector.h"  // vec_CalcValue

void calc_value_free(CalcValue this) {
  str_free(this.name);
  expr_value_free(this.value);
}

CalcValue calc_value_clone(const CalcValue* this) {
  return (CalcValue){.name = str_clone(&this->name),
                     .value = expr_value_clone(&this->value)};
}

void calc_value_print(const CalcValue* this, OutStream stream) {
  x_sprintf(stream, "CalcValue(%s = %$expr_value)", this->name.string,
            this->value);
}