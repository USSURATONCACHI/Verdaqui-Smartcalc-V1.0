#include "credit_tab.h"

#include <float.h>
#include <math.h>

#include "../calculator/credit_deposit.h"
#include "../util/allocator.h"
#include "../util/better_string.h"
#include "../util/prettify_c.h"

CreditTab credit_tab_create() {
  return (CreditTab){
      .credit_type = CREDIT_ANNUITY,

      .amount = 1000.0,
      .interest_monthly = 10.0,

      .duration = 12,
      .duration_type = DUR_MONTHS,
  };
}
void credit_tab_free(CreditTab tab) {
  unused(tab);
  // nothing
}

static void credit_nk_option(int* value, struct nk_context* ctx,
                             const char* name, int type) {
  if (nk_option_text(ctx, name, strlen(name), (*value)is type)) (*value) = type;
}

static void draw_annuity_credit(CreditTab* this, struct nk_context* ctx);
static void draw_differentiated_credit(CreditTab* this, struct nk_context* ctx);

void credit_tab_draw(CreditTab* this, struct nk_context* ctx,
                     GLFWwindow* window) {
  unused(window);
  nk_layout_row_dynamic(ctx, 30, 1);
  nk_label(ctx, "Credit", NK_TEXT_ALIGN_LEFT);

  nk_layout_row_dynamic(ctx, 30, 2);
  credit_nk_option(&this->credit_type, ctx, "Annuity", CREDIT_ANNUITY);
  credit_nk_option(&this->credit_type, ctx, "Differentiated",
                   CREDIT_DIFFERENTIATED);

  nk_layout_row_dynamic(ctx, 30, 1);
  nk_property_double(ctx, "Amount", 0.0, &this->amount, 10.0e100, 0.0,
                     this->amount / 100.0 + 1.0);

  nk_layout_row_dynamic(ctx, 30, 1);
  nk_property_double(ctx, "Interest rate (%/mo)", 0.0, &this->interest_monthly,
                     100.0, 0.0, 1.0);

  nk_layout_row_dynamic(ctx, 30, 3);
  nk_property_int(ctx, "Duration (term)", 0, &this->duration, 10000, 0, 1.0);
  credit_nk_option(&this->duration_type, ctx, "Months", DUR_MONTHS);
  credit_nk_option(&this->duration_type, ctx, "Years", DUR_YEARS);

  if (this->credit_type is CREDIT_ANNUITY) {
    draw_annuity_credit(this, ctx);
  } else if (this->credit_type is CREDIT_DIFFERENTIATED) {
    draw_differentiated_credit(this, ctx);
  } else {
    panic("Unknown credit type");
  }
}

static void draw_annuity_credit(CreditTab* this, struct nk_context* ctx) {
  int dur_monthly =
      this->duration * (this->duration_type is DUR_YEARS ? 12 : 1);

  AnnuityResult res = calculate_annuity_credit(
      dur_monthly, this->interest_monthly, this->amount);

  str_t payment_text = str_owned("Montly payment: %lf", res.payment_monthly);
  str_t total_text = str_owned("Total repayment: %lf", res.total_repayment);
  str_t overpayment_text = str_owned("Overpayment: %lf", res.overpayment);

  nk_layout_row_dynamic(ctx, 30, 1);
  nk_label(ctx, payment_text.string, NK_TEXT_ALIGN_LEFT);
  nk_label(ctx, total_text.string, NK_TEXT_ALIGN_LEFT);
  nk_label(ctx, overpayment_text.string, NK_TEXT_ALIGN_LEFT);

  str_free(payment_text);
  str_free(total_text);
  str_free(overpayment_text);
}

static void draw_differentiated_credit(CreditTab* this,
                                       struct nk_context* ctx) {
  int dur_monthly =
      this->duration * (this->duration_type is DUR_YEARS ? 12 : 1);
  DifferentiatedResult result = calculate_differentiated_credit(
      dur_monthly, this->interest_monthly, this->amount);

  str_t payment_text = str_owned("Montly payment: %lf .. %lf",
                                 result.first_payment, result.last_payment);
  str_t total_text = str_owned("Total repayment: %lf", result.total_repayment);
  str_t overpayment_text = str_owned("Overpayment: %lf", result.overpayment);

  nk_layout_row_dynamic(ctx, 30, 1);
  nk_label(ctx, payment_text.string, NK_TEXT_ALIGN_LEFT);
  nk_label(ctx, total_text.string, NK_TEXT_ALIGN_LEFT);
  nk_label(ctx, overpayment_text.string, NK_TEXT_ALIGN_LEFT);

  str_free(payment_text);
  str_free(total_text);
  str_free(overpayment_text);
}

void credit_tab_update(CreditTab* this) { unused(this); }

void credit_tab_on_scroll(CreditTab* this, double x, double y) {
  unused(this);
  unused(x);
  unused(y);
}
void credit_tab_on_mouse_move(CreditTab* this, double x, double y) {
  unused(this);
  unused(x);
  unused(y);
}
void credit_tab_on_mouse_click(CreditTab* this, int button, int action,
                               int mods) {
  unused(this);
  unused(button);
  unused(action);
  unused(mods);
}