#include "app.h"

#include <string.h>

#include "util/prettify_c.h"

App* app_create() {
  debugln("Creating app...");
  App* result = (App*)malloc(sizeof(App));
  assert_alloc(result);

  (*result) = (App){
      .current_tab = 0,

      .graphing = graphing_tab_create(),
      .credit = null,
      .deposit = null,
  };
  return result;
}

void app_free(App* app) {
  debugln("Freeing the graphing tab");
  graphing_tab_free(app->graphing);
  // credit_tab_free(app->credit);
  // deposit_tab_free(app->deposit);
  free(app);
}

void app_render(App* this, struct nk_context* ctx, GLFWwindow* window) {
  const char* tabs_names[] = TABS_NAMES;
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  // glClearColor(1.0, 1.0, 1.0, 1.0);
  // glClear(GL_COLOR_BUFFER_BIT);

  if (nk_begin(ctx, "Main window", nk_rect(0, 0, 500, height), 0)) {
    nk_layout_row_dynamic(ctx, 30, 2);
    for (int i = 0; i < TABS_COUNT; i++)
      if (nk_option_text(ctx, tabs_names[i], strlen(tabs_names[i]),
                         this->current_tab == i))
        this->current_tab = i;

    switch (this->current_tab) {
      case 0:
        graphing_tab_draw(this->graphing, ctx, window);
        break;
    }
  }
  nk_end(ctx);
}

void app_on_scroll(App* this, double x, double y) {
  switch (this->current_tab) {
    case 0:
      graphing_tab_on_scroll(this->graphing, x, y);
      break;
  }
}

void app_on_mouse_move(App* this, double pos_x, double pos_y) {
  switch (this->current_tab) {
    case 0:
      graphing_tab_on_mouse_move(this->graphing, pos_x, pos_y);
      break;
  }
}

void app_on_mouse_click(App* this, int button, int action, int mods) {
  switch (this->current_tab) {
    case 0:
      graphing_tab_on_mouse_click(this->graphing, button, action, mods);
      break;
  }
}

void app_update(App* this) {
  switch (this->current_tab) {
    case 0:
      graphing_tab_update(this->graphing);
      break;
  }
}