#ifndef CALCULATOR_CREDIT_DEPOSIT_
#define CALCULATOR_CREDIT_DEPOSIT_

typedef struct AnnuityResult {
  double payment_monthly;
  double total_repayment;
  double overpayment;
} AnnuityResult;

AnnuityResult calculate_annuity_credit(int dur_months, double interest_rate,
                                       double amount);

typedef struct DifferentiatedResult {
  double first_payment;
  double last_payment;
  double total_repayment;
  double overpayment;
} DifferentiatedResult;

DifferentiatedResult calculate_differentiated_credit(int dur_monthly,
                                                     double interest_rate,
                                                     double amount);

#endif  // CALCULATOR_CREDIT_DEPOSIT_
