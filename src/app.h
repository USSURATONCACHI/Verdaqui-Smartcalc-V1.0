#ifndef SRC_APP_H_
#define SRC_APP_H_

#include <stdbool.h>

#include "full_nuklear.h"
#include "ui/graphing_tab.h"

#define TABS_NAMES \
  { "Graphic calculator", "Deposit calculator", "Credit calculator" }
#define TABS_COUNT 3

#define TAB_GRAPHING 0
#define TAB_DEPOSIT 1
#define TAB_CREDIT 2

typedef struct DepositTab DepositTab;
typedef struct CreditTab CreditTab;

typedef struct App {
  int current_tab;

  GraphingTab* graphing;
  DepositTab* deposit;
  CreditTab* credit;
} App;

App* app_create();
void app_free(App*);

void app_render(App*, struct nk_context* ctx, GLFWwindow* window);
void app_update(App*);

void app_on_scroll(App* this, double x, double y);
void app_on_mouse_move(App* this, double pos_x, double pos_y);
void app_on_mouse_click(App* this, int button, int action, int mods);

#endif  // SRC_APP_H_