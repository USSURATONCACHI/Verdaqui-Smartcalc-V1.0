#include "credit_deposit.h"

#include <math.h>

#include "../util/prettify_c.h"

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