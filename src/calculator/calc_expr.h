#ifndef SRC_CALCULATOR_CALC_EXPR_H_
#define SRC_CALCULATOR_CALC_EXPR_H_

#include "../parser/expr.h"
#include "../util/better_io.h"

#define CALC_EXPR_VARIABLE 20  // name
#define CALC_EXPR_FUNCTION 21  // name, args
#define CALC_EXPR_PLOT 22      // just expression
#define CALC_EXPR_ACTION 23

typedef struct CalcExpr {
  Expr expression;
  int type;
  union {
    str_t variable_name;

    struct {
      str_t name;
      vec_str_t args;
    } function;

    // plot - nothing
    // action - nothing
  };
} CalcExpr;

typedef struct CalcExprResult {
  bool is_ok;
  union {
    CalcExpr ok;
    struct {
      const char* err_pos;
      str_t err_text;
    };
  };
} CalcExprResult;

#define CalcExprOk(val) \
  (CalcExprResult) { .is_ok = true, .ok = (val) }
#define CalcExprErr(pos, text) \
  (CalcExprResult) { .is_ok = false, .err_pos = (pos), .err_text = (text) }

void calc_expr_free(CalcExpr this);
CalcExpr calc_expr_clone(const CalcExpr* source);
const char* calc_expr_type_text(int type);
void calc_expr_print(const CalcExpr* this, OutStream stream);

CalcExprResult calc_expr_parse(ExprContext ctx, const char* text);
CalcExprResult calc_expr_parse_tt(ExprContext ctx, TokenTree tree);

#define VECTOR_H CalcExpr
#include "../util/vector.h"

#endif  // SRC_CALCULATOR_CALC_EXPR_H_