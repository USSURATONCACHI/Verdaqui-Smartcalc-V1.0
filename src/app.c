#include "app.h"

#include <string.h>

#include "util/prettify_c.h"

#define SIDEBAR_WIDTH 500

App* app_create(int screen_w, int screen_h) {
  debugln("Creating app...");
  App* result = (App*)malloc(sizeof(App));
  assert_alloc(result);

  (*result) = (App){
      .current_tab = TAB_GRAPHING,

      .graphing = graphing_tab_create(screen_w, screen_h),
      .credit = credit_tab_create(),
      .deposit = deposit_tab_create(),

      .last_mouse_x = 0.0,
      .last_mouse_y = 0.0,
  };
  return result;
}

void app_free(App* app) {
  debugln("Freeing the graphing tab");
  graphing_tab_free(app->graphing);
  credit_tab_free(app->credit);
  deposit_tab_free(app->deposit);
  free(app);
}

void app_render(App* this, struct nk_context* ctx, GLFWwindow* window) {
  const char* tabs_names[] = TABS_NAMES;
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  if (width is 0 or height is 0)
    return;

  int menu_width = this->current_tab is TAB_GRAPHING ? SIDEBAR_WIDTH : width;

  if (nk_begin(ctx, "Main window", nk_rect(0, 0, menu_width, height), 0)) {
    nk_layout_row_dynamic(ctx, 30, 2);
    for (int i = 0; i < TABS_COUNT; i++)
      if (nk_option_text(ctx, tabs_names[i], strlen(tabs_names[i]),
                         this->current_tab == i))
        this->current_tab = i;

    switch (this->current_tab) {
      case TAB_GRAPHING:
        graphing_tab_draw(this->graphing, ctx, window);
        break;

      case TAB_CREDIT:
        credit_tab_draw(&this->credit, ctx, window);
        break;

      case TAB_DEPOSIT:
        deposit_tab_draw(&this->deposit, ctx, window);
        break;
    }
  }
  nk_end(ctx);
}

void app_on_scroll(App* this, double x, double y) {
  if (this->last_mouse_x > SIDEBAR_WIDTH)
  switch (this->current_tab) {
    case TAB_GRAPHING:
      graphing_tab_on_scroll(this->graphing, x, y);
      break;
    case TAB_CREDIT:
      credit_tab_on_scroll(&this->credit, x, y);
      break;
    case TAB_DEPOSIT:
      deposit_tab_on_scroll(&this->deposit, x, y);
      break;
  }
}

void app_on_mouse_move(App* this, double pos_x, double pos_y) {
  this->last_mouse_x = pos_x;
  this->last_mouse_y = pos_y;
  switch (this->current_tab) {
    case TAB_GRAPHING:
      graphing_tab_on_mouse_move(this->graphing, pos_x, pos_y);
      break;
    case TAB_CREDIT:
      credit_tab_on_mouse_move(&this->credit, pos_x, pos_y);
      break;
    case TAB_DEPOSIT:
      deposit_tab_on_mouse_move(&this->deposit, pos_x, pos_y);
      break;
  }
}

void app_on_mouse_click(App* this, int button, int action, int mods) {
  switch (this->current_tab) {
    case TAB_GRAPHING:
      graphing_tab_on_mouse_click(this->graphing, button, action, mods);
      break;
    case TAB_CREDIT:
      credit_tab_on_mouse_click(&this->credit, button, action, mods);
      break;
    case TAB_DEPOSIT:
      deposit_tab_on_mouse_click(&this->deposit, button, action, mods);
      break;
  }
}

void app_update(App* this) {
  switch (this->current_tab) {
    case TAB_GRAPHING:
      graphing_tab_update(this->graphing);
      break;
    case TAB_CREDIT:
      credit_tab_update(&this->credit);
      break;
    case TAB_DEPOSIT:
      deposit_tab_update(&this->deposit);
      break;
  }
}