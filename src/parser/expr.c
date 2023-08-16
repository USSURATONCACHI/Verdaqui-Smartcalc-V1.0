
#include "expr.h"

#include "../util/allocator.h"

#define VECTOR_C Expr
#define VECTOR_ITEM_DESTRUCTOR expr_free
#define VECTOR_ITEM_CLONE expr_clone
#include "../util/vector.h"

#define VECTOR_C vec_Expr
#define VECTOR_ITEM_DESTRUCTOR vec_Expr_free
#define VECTOR_ITEM_CLONE vec_Expr_clone
#include "../util/vector.h"

// FUNCTIONS

// -- Basic functionality

// =====
// =
// = expr_free
// =
// =====
void expr_free(Expr this) {
  if (this.type is EXPR_NUMBER) {
    // Number does not OWN any resources to free
  } else if (this.type is EXPR_VARIABLE) {
    str_free(this.variable.name);

  } else if (this.type is EXPR_FUNCTION) {
    str_free(this.function.name);
    if (this.function.argument) {
      expr_free(*this.function.argument);
      FREE(this.function.argument);
    }

  } else if (this.type is EXPR_VECTOR) {
    vec_Expr_free(this.vector.arguments);

  } else if (this.type is EXPR_BINARY_OP) {
    str_free(this.binary_operator.name);

    if (this.binary_operator.lhs) expr_free(*this.binary_operator.lhs);
    if (this.binary_operator.rhs) expr_free(*this.binary_operator.rhs);
    FREE(this.binary_operator.lhs);
    FREE(this.binary_operator.rhs);

  } else {
    panic("Invalid expr type");
  }
}

// =====
// =
// = expr_print
// =
// =====
static void expr_print_binary_op(const Expr* this, OutStream out);
void expr_print(const Expr* this, OutStream out) {
  if (this is null) {
    x_sprintf(out, "<nullptr>");
    return;
  } else if (this->type is EXPR_NUMBER) {
    x_sprintf(out, "%.1lf", this->number.value);

  } else if (this->type is EXPR_VARIABLE) {
    x_sprintf(out, "%s", this->variable.name.string);
  } else if (this->type is EXPR_FUNCTION) {
    x_sprintf(out, "<%s> ", this->function.name.string);
    expr_print(this->function.argument, out);

  } else if (this->type is EXPR_VECTOR) {
    outstream_putc('[', out);
    for (int i = 0; i < this->vector.arguments.length; i++) {
      if (i > 0) x_sprintf(out, ", ");
      expr_print(&this->vector.arguments.data[i], out);
    }
    outstream_putc(']', out);

  } else if (this->type is EXPR_BINARY_OP) {
    expr_print_binary_op(this, out);

  } else {
    panic("Invalid expr type");
  }
}

static void expr_print_binary_op(const Expr* this, OutStream out) {
  assert_m(this->type is EXPR_BINARY_OP);

  outstream_putc('(', out);
  expr_print(this->binary_operator.lhs, out);

  if (strcmp(this->binary_operator.name.string, "[]") is 0) {
    // Indexing[operator]
    outstream_putc('[', out);
    expr_print(this->binary_operator.rhs, out);
    outstream_putc(']', out);
  } else {
    // Regular + operator
    x_sprintf(out, " %s ", this->binary_operator.name.string);
    expr_print(this->binary_operator.rhs, out);
  }

  outstream_putc(')', out);
}

// =====
// =
// = expr_clone
// =
// =====
Expr expr_clone(const Expr* this) {
  assert_m(this);

  Expr result;
  if (this->type is EXPR_NUMBER) {
    result = *this;

  } else if (this->type is EXPR_VARIABLE) {
    result = (Expr){
        .type = EXPR_VARIABLE,
        .variable.name = str_clone(&this->variable.name),
    };

  } else if (this->type is EXPR_FUNCTION) {
    result = (Expr){.type = EXPR_FUNCTION,
                    .function = {.argument = expr_move_to_heap(
                                     expr_clone(this->function.argument)),
                                 .name = str_clone(&this->function.name)}};

  } else if (this->type is EXPR_VECTOR) {
    result = (Expr){
        .type = EXPR_VECTOR,
        .vector.arguments = vec_Expr_clone(&this->vector.arguments),
    };

  } else if (this->type is EXPR_BINARY_OP) {
    result = (Expr){
        .type = EXPR_BINARY_OP,
        .binary_operator = {
            .name = str_clone(&this->binary_operator.name),
            .lhs = expr_move_to_heap(expr_clone(this->binary_operator.lhs)),
            .rhs = expr_move_to_heap(expr_clone(this->binary_operator.rhs)),
        }};

  } else {
    panic("Invalid expr type");
  }

  return result;
}

// =====
// =
// = expr_move_to_heap
// =
// =====
Expr* expr_move_to_heap(Expr value) {
  Expr* ptr = (Expr*)MALLOC(sizeof(Expr));
  assert_alloc(ptr);
  (*ptr) = value;
  return ptr;
}

// =====
// =
// = expr_type_text
// =
// =====
const char* expr_type_text(int type) {
  if (type is EXPR_NUMBER)
    return "Number";
  else if (type is EXPR_VARIABLE)
    return "Variable";
  else if (type is EXPR_FUNCTION)
    return "Function";
  else if (type is EXPR_VECTOR)
    return "Vector";
  else if (type is EXPR_BINARY_OP)
    return "Binary operator";
  else
    panic("Unknown Expr type");
}

/*

vec_str_t expr_get_used_variables(const Expr* this) {
  vec_str_t result = vec_str_t_create();

  if (this->type is EXPR_FUNCTION or this->type is EXPR_NUMBER) {
    // nothing
  } else if (this->type is EXPR_VARIABLE) {
    vec_str_t_push(&result, str_clone(&this->variable.name));

  } else if (this->type is EXPR_VECTOR) {
    for (int el = 0; el < this->vector.arguments.length; el++) {
      vec_str_t variables =
          expr_get_used_variables(&this->vector.arguments.data[el]);
      vstrt_push_unique(&result, variables);
    }

  } else if (this->type is EXPR_BINARY_OP) {
    vstrt_push_unique(&result,
                      expr_get_used_variables(this->binary_operator.lhs));
    vstrt_push_unique(&result,
                      expr_get_used_variables(this->binary_operator.rhs));

  } else {
    panic("Unknown expr type");
  }

  return result;
}

vec_str_t expr_get_used_functions(const Expr* this) {
  vec_str_t result = vec_str_t_create();

  if (this->type is EXPR_VARIABLE or this->type is EXPR_NUMBER) {
    // nothing
  } else if (this->type is EXPR_FUNCTION) {
    vec_str_t_push(&result, str_clone(&this->function.name));

  } else if (this->type is EXPR_VECTOR) {
    for (int el = 0; el < this->vector.arguments.length; el++) {
      vec_str_t variables =
          expr_get_used_functions(&this->vector.arguments.data[el]);
      vstrt_push_unique(&result, variables);
    }

  } else if (this->type is EXPR_BINARY_OP) {
    vstrt_push_unique(&result,
                      expr_get_used_functions(this->binary_operator.lhs));
    vstrt_push_unique(&result,
                      expr_get_used_functions(this->binary_operator.rhs));

  } else {
    panic("Unknown expr type");
  }

  return result;
}

void expr_iter_variables(const Expr* this,
                         void (*callback)(void* cb_data, const str_t* var_name),
                         void* cb_data) {
  if (this->type is EXPR_VARIABLE) {
    callback(cb_data, &this->variable.name);
  } else if (this->type is EXPR_NUMBER) {
  } else if (this->type is EXPR_VECTOR) {
    for (int i = 0; i < this->vector.arguments.length; i++)
      expr_iter_variables(&this->vector.arguments.data[i], callback, cb_data);
  } else if (this->type is EXPR_BINARY_OP) {
    expr_iter_variables(this->binary_operator.lhs, callback, cb_data);
    expr_iter_variables(this->binary_operator.rhs, callback, cb_data);
  } else if (this->type is EXPR_FUNCTION) {
    expr_iter_variables(this->function.argument, callback, cb_data);
  } else {
    panic("Unknown expr type");
  }
}

*/