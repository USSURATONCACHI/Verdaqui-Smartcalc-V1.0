#ifndef SRC_CODE_UI_GRAPHING_TAB_H_
#define SRC_CODE_UI_GRAPHING_TAB_H_

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "../calculator/calculator.h"
#include "../nuklear_flags.h"
#include "../util/camera.h"
#include "../util/mesh.h"

typedef struct GraphingTab GraphingTab;

typedef struct ui_expr {
  struct nk_text_edit textedit;
  struct nk_colorf color;

  bool prev_active;
  str_t descr_text;
} ui_expr_t;
ui_expr_t ui_expr_create(const char* text);
void ui_expr_update(GraphingTab* gt, ui_expr_t* this);
void ui_expr_free(ui_expr_t this);

// vec_ui_expr
typedef ui_expr_t ui_expr;
#define VECTOR_H ui_expr
#include "../util/vector.h"

#define ICON_HOME 0
#define ICON_CROSS 1
#define ICON_PLUS 2
#define ICONS_COUNT 3

struct GraphingTab {
  GLuint plot_shader;
  Mesh square_mesh;

  double last_mouse_x, last_mouse_y;
  bool is_dragging;
  PlotCamera camera;

  vec_ui_expr expressions;
  CalcBackend calc;

  struct nk_image icons[ICONS_COUNT];
};

GraphingTab* graphing_tab_create();
void graphing_tab_free(GraphingTab*);
void graphing_tab_update(GraphingTab* this);
void graphing_tab_update_calc(GraphingTab* this);
void graphing_tab_draw(GraphingTab* this, struct nk_context* ctx,
                       GLFWwindow* window);

void graphing_tab_on_scroll(GraphingTab* this, double x, double y);
void graphing_tab_on_mouse_move(GraphingTab* this, double x, double y);
void graphing_tab_on_mouse_click(GraphingTab* this, int button, int action,
                                 int mods);

#endif  // SRC_CODE_UI_GRAPHING_TAB_H_