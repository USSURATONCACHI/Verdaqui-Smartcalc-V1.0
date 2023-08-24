#include "deposit_tab.h"

#include <stdbool.h>

#include "../util/better_string.h"
#include "icon_load.h"

DepositTab deposit_tab_create() {
  DepositTab result = {
      .amount = 1000.0,
      .duration = 12,
      .duration_type = DEPOSIT_DUR_MONTHS,
      .interest_rate = 12.0,
      .tax_rate = 0.0,

      .capit_period = 1,
      .capitalization = false,

      .placements = vec_Placement_create(),
      .withdrawals = vec_Placement_create(),

      .icon_plus = load_nk_icon("assets/img/plus.png"),
      .icon_cross = load_nk_icon("assets/img/close.png"),
  };

  return result;
}
void deposit_tab_free(DepositTab tab) {
  vec_Placement_free(tab.placements);
  vec_Placement_free(tab.withdrawals);
  delete_nk_icon(tab.icon_plus);
  delete_nk_icon(tab.icon_cross);
}

static void deposit_nk_option(int* value, struct nk_context* ctx,
                              const char* name, int type) {
  if (nk_option_text(ctx, name, strlen(name), (*value)is type)) (*value) = type;
}

static void draw_deposit_result(DepositTab* this, struct nk_context* ctx);

static void draw_placements(char p_letter, struct nk_context* ctx,
                            vec_Placement* placements, int max_dur,
                            struct nk_image plus_icon,
                            struct nk_image cross_icon);

void deposit_tab_draw(DepositTab* this, struct nk_context* ctx,
                      GLFWwindow* window) {
  unused(window);
  nk_layout_row_dynamic(ctx, 30, 1);
  nk_label(ctx, "Deposit", NK_TEXT_ALIGN_LEFT);

  nk_layout_row_dynamic(ctx, 30, 1);
  nk_property_double(ctx, "Amount", 0.0, &this->amount, 10.0e100, 0.0,
                     this->amount / 100.0 + 1.0);

  nk_property_double(ctx, "Interest rate (%/mo)", 0.0, &this->interest_rate,
                     100.0, 0.0, 1.0);

  nk_property_double(ctx, "Tax rate (%/payment)", 0.0, &this->tax_rate, 100.0,
                     0.0, 1.0);

  nk_layout_row_dynamic(ctx, 30, 3);
  nk_property_int(ctx, "Duration (term)", 0, &this->duration, 10000, 0, 1.0);
  deposit_nk_option(&this->duration_type, ctx, "Months", DEPOSIT_DUR_MONTHS);
  deposit_nk_option(&this->duration_type, ctx, "Years", DEPOSIT_DUR_YEARS);

  nk_layout_row_dynamic(ctx, 30, 1);
  this->capitalization =
      nk_check_label(ctx, "Capitalization", this->capitalization);

  nk_property_int(ctx, "Capitalization period (months)", 1, &this->capit_period,
                  360, 0, 1);

  nk_layout_row_dynamic(ctx, 30, 1);
  nk_label(ctx, "Placements", NK_TEXT_ALIGN_LEFT);
  draw_placements('P', ctx, &this->placements,
                  this->duration * this->duration_type, this->icon_plus,
                  this->icon_cross);

  nk_layout_row_dynamic(ctx, 30, 1);
  nk_label(ctx, "Withdrawals", NK_TEXT_ALIGN_LEFT);
  draw_placements('W', ctx, &this->withdrawals,
                  this->duration * this->duration_type, this->icon_plus,
                  this->icon_cross);

  draw_deposit_result(this, ctx);
}

static void draw_placements(char p_letter, struct nk_context* ctx,
                            vec_Placement* placements, int max_dur,
                            struct nk_image plus_icon,
                            struct nk_image cross_icon) {
  for (int i = 0; i < placements->length; i++) {
    Placement* item = &placements->data[i];
    nk_layout_row_begin(ctx, NK_STATIC, 25, 3);

    nk_layout_row_push(ctx, 25.0);
    if (nk_button_image(ctx, cross_icon)) {
      vec_Placement_delete_order(placements, i--);
      continue;
    }

    str_t amount_name = str_owned("Amount (%c%d)", p_letter, i + 1);
    str_t month_name = str_owned("Month (%c%d)", p_letter, i + 1);

    nk_layout_row_push(ctx, 300.0);
    nk_property_int(ctx, month_name.string, 0, &item->month, max_dur - 1, 0,
                    1.0);

    nk_layout_row_push(ctx, 300.0);
    nk_property_double(ctx, amount_name.string, 0.0, &item->amount, 1.0e20, 0.0,
                       item->amount / 100.0 + 1.0);

    str_free(amount_name);
    str_free(month_name);
  }

  nk_layout_row_static(ctx, 25, 25, 1);
  if (nk_button_image(ctx, plus_icon)) {
    Placement p = {.month = 0, .amount = 1000.0};
    vec_Placement_push(placements, p);
  }
}

static void draw_deposit_result(DepositTab* this, struct nk_context* ctx) {
  DepositResult res = calculate_deposit((DepositInfo){
      .amount = this->amount,
      .capitalization = this->capitalization,
      .capitalization_period = this->capit_period,
      .duration_months = this->duration * this->duration_type,
      .interest_rate = this->interest_rate,
      .placements = &this->placements,
      .tax_rate = this->tax_rate,
      .withdrawals = &this->withdrawals,
  });

  str_t tax_sum = str_owned("Tax sum: %.2lf", res.tax_sum);
  str_t deposited_amount =
      str_owned("Deposited amount: %.2lf", res.deposit_amount);
  str_t total_amount = str_owned("Total amount: %.2lf", res.total_amount);
  str_t accured_interest = str_owned("Accured interest (%%/mo): %.2lf",
                                     res.accured_interest * 100.0);

  nk_layout_row_dynamic(ctx, 30, 1);
  nk_label(ctx, tax_sum.string, NK_TEXT_ALIGN_LEFT);
  nk_label(ctx, deposited_amount.string, NK_TEXT_ALIGN_LEFT);
  nk_label(ctx, total_amount.string, NK_TEXT_ALIGN_LEFT);
  nk_label(ctx, accured_interest.string, NK_TEXT_ALIGN_LEFT);

  str_free(tax_sum);
  str_free(deposited_amount);
  str_free(total_amount);
  str_free(accured_interest);
}

void deposit_tab_update(DepositTab* this) { unused(this); }

void deposit_tab_on_scroll(DepositTab* this, double x, double y) {
  unused(this);
  unused(x);
  unused(y);
}
void deposit_tab_on_mouse_move(DepositTab* this, double x, double y) {
  unused(this);
  unused(x);
  unused(y);
}
void deposit_tab_on_mouse_click(DepositTab* this, int button, int action,
                                int mods) {
  unused(this);
  unused(button);
  unused(action);
  unused(mods);
}