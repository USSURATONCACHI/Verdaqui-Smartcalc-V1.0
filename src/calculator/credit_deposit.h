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

#define DEPOSIT_DUR_MONTHS 1
#define DEPOSIT_DUR_YEARS 12

typedef struct Placement {
  int month;
  double amount;
} Placement;

#define VECTOR_H Placement
#include "../util/vector.h"  // vec_Placement

typedef struct DepositResult {
  double deposit_amount;
  double accured_interest;
  double tax_sum;
  double total_amount;
} DepositResult;

typedef struct DepositInfo {
  int duration_months;
  double interest_rate, amount, tax_rate;
  vec_Placement *placements, *withdrawals;

  bool capitalization;
  int capitalization_period;
} DepositInfo;

DepositResult calculate_deposit(DepositInfo info);

#endif  // CALCULATOR_CREDIT_DEPOSIT_
