#include "../util/allocator.h"
#include "../util/prettify_c.h"
#include "calc_backend.h"
#include "func_const_ctx.h"
#include "native_functions.h"

str_t calc_backend_add_expr(CalcBackend* this, const char* text) {
  ExprContext ctx = calc_backend_get_context(this);
  debugln("Trying to parse...");
  CalcExprResult res = calc_expr_parse(ctx, text);
  debugln("Parsed : %d", res.is_ok);

  str_t message = str_literal("");
  if (res.is_ok) {
    // message = str_owned("Ok (%s)", );
    int type = res.ok.type;
    const char* type_text = calc_expr_type_text(type);

    debugln("Type is %s, is const is: %b", calc_expr_type_text(type),
            calc_backend_is_expr_const(this, &res.ok.expression));
    if ((type is CALC_EXPR_VARIABLE or type is CALC_EXPR_PLOT) and
        calc_backend_is_expr_const(this, &res.ok.expression)) {
      ExprContext ctx = calc_backend_get_context(this);
      ExprValueResult val_res = expr_calculate(&res.ok.expression, ctx);
      if (val_res.is_ok) {
        message = str_owned("%$expr_value", val_res.ok);
        expr_value_free(val_res.ok);
      } else {
        message = str_owned("Err: %s", val_res.err_text.string);
        str_free(val_res.err_text);
      }
    } else if (type is CALC_EXPR_FUNCTION and
               calc_backend_is_func_const_ptr(this, &res.ok)) {
      message = str_owned("Const function");
    }

    if (strlen(message.string) is 0) {
      str_free(message);
      message = str_owned("%s", type_text);
    }
    vec_CalcExpr_push(&this->expressions, res.ok);
  } else {
    str_free(message);
    if (res.err_pos) {
      message = str_owned("Err (at '%.10s'): %s", res.err_pos, res.err_text);
    } else {
      message = str_owned("Err: %s", res.err_text.string);
    }
    str_free(res.err_text);
  }

  return message;
}