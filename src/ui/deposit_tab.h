#ifndef UI_DEPOSIT_TAB_
#define UI_DEPOSIT_TAB_

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "../nuklear_flags.h"

#define DEPOSIT_DUR_MONTHS 1
#define DEPOSIT_DUR_YEARS 12

typedef struct Placement {
  int month;
  double amount;
} Placement;

#define VECTOR_H Placement
#include "../util/vector.h"  // vec_Placement

typedef struct DepositTab {
  double amount;

  int duration;
  int duration_type;

  double interest_rate;
  double tax_rate;

  bool capitalization;
  int capit_period;

  vec_Placement placements;
  vec_Placement withdrawals;

  struct nk_image icon_plus;
  struct nk_image icon_cross;
} DepositTab;

DepositTab deposit_tab_create();
void deposit_tab_free(DepositTab tab);
void deposit_tab_draw(DepositTab* this, struct nk_context* ctx,
                      GLFWwindow* window);
void deposit_tab_update(DepositTab* this);

void deposit_tab_on_scroll(DepositTab* this, double x, double y);
void deposit_tab_on_mouse_move(DepositTab* this, double x, double y);
void deposit_tab_on_mouse_click(DepositTab* this, int button, int action,
                                int mods);

#endif  // UI_DEPOSIT_TAB_
