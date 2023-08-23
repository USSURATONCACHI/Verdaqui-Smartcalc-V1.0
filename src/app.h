#ifndef SRC_APP_H_
#define SRC_APP_H_

#include <stdbool.h>

#include "full_nuklear.h"
#include "ui/graphing_tab.h"
#include "ui/credit_tab.h"
#include "ui/deposit_tab.h"
#include "ui/classic_tab.h"

#define TABS_NAMES \
  { "Graphic calculator", "Credit calculator", "Classic calculator", "Deposit calculator" }
#define TABS_COUNT 4

#define TAB_GRAPHING 0
#define TAB_CREDIT 1
#define TAB_CLASSIC 2
#define TAB_DEPOSIT 3


typedef struct App {
  int current_tab;

  GraphingTab* graphing;
  CreditTab credit;
  DepositTab deposit;
  ClassicTab classic;

  double last_mouse_x, last_mouse_y;
} App;

App* app_create(int screen_w, int screen_h);
void app_free(App*);

void app_render(App*, struct nk_context* ctx, GLFWwindow* window);
void app_update(App*);

void app_on_scroll(App* this, double x, double y);
void app_on_mouse_move(App* this, double pos_x, double pos_y);
void app_on_mouse_click(App* this, int button, int action, int mods);

#endif  // SRC_APP_H_