#include "credit_deposit.h"

#include <math.h>

#include "../util/prettify_c.h"

#define VECTOR_C Placement
#include "../util/vector.h"  // vec_Placement

AnnuityResult calculate_annuity_credit(int dur_monthly, double interest_rate,
                                       double amount) {
  double rate = interest_rate / 100.0;
  double coef = pow(1.0 + rate, (double)dur_monthly);

  double payment_monthly = amount * (rate * coef) / (coef - 1);
  double total_repayment = payment_monthly * (double)dur_monthly;
  double overpayment = total_repayment - amount;

  return (AnnuityResult){
      .payment_monthly = payment_monthly,
      .total_repayment = total_repayment,
      .overpayment = overpayment,
  };
}

DifferentiatedResult calculate_differentiated_credit(int dur_monthly,
                                                     double interest_rate,
                                                     double amount) {
  double rate = interest_rate / 100.0;

  double total_repayment = 0.0;
  double first_payment = 0.0;
  double last_payment = 0.0;
  for (int i = 1; i <= dur_monthly; i++) {
    double monthly = amount / dur_monthly +
                     rate * (amount - (amount / dur_monthly) * (i - 1));
    total_repayment += monthly;

    if (i is 1) first_payment = monthly;
    if (i is dur_monthly) last_payment = monthly;
  }
  double overpayment = total_repayment - amount;

  return (DifferentiatedResult){
      .last_payment = last_payment,
      .first_payment = first_payment,
      .overpayment = overpayment,
      .total_repayment = total_repayment,
  };
}

DepositResult calculate_deposit(DepositInfo info) {
  double rate = info.interest_rate / 100.0;
  double deposit = info.amount;
  double payment = 0.0;
  double tax_sum = 0.0;

  double tax_coef = 1.0 - info.tax_rate / 100.0;

  for (int i = 0; i < info.duration_months; i++) {
    double cur_payment = deposit * rate;
    payment += cur_payment * tax_coef;
    tax_sum += cur_payment * info.tax_rate / 100.0;

    bool capitalization_end = ((i + 1) % info.capitalization_period) is 0 or
                              (i + 1) is info.duration_months;
    if (info.capitalization and capitalization_end) {
      deposit += payment;
      payment = 0.0;
    }

    for (int j = 0; j < info.placements->length; j++)
      if (info.placements->data[j].month is i)
        deposit += info.placements->data[j].amount;

    for (int j = 0; j < info.withdrawals->length; j++)
      if (info.withdrawals->data[j].month is i)
        deposit -= info.withdrawals->data[j].amount;
  }

  double total_amount = deposit + payment * tax_coef;
  double accured_interest =
      (total_amount / info.amount - 1.0) / info.duration_months;

  return (DepositResult){
      .tax_sum = tax_sum,
      .accured_interest = accured_interest,
      .deposit_amount = deposit,
      .total_amount = total_amount,
  };
}