#ifndef SRC_CODE_UI_GRAPHING_TAB_H_
#define SRC_CODE_UI_GRAPHING_TAB_H_

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "../nuklear_flags.h"
#include "../util/camera.h"
#include "mesh.h"
#include "framebuffer.h"
#include "shader_loader.h"
#include "ui_expr.h"

#define ICON_HOME 0
#define ICON_CROSS 1
#define ICON_PLUS 2
#define ICONS_COUNT 3

#define MULTISAMPLES 4

#define GRAPHING_MAX_SHADERS 10000

typedef struct NamedShader {
  str_t name;
  GlProgram shader;
} NamedShader;

void named_shader_free(NamedShader ns);

#define VECTOR_H NamedShader
#include "../util/vector.h"

typedef struct Plot {
  GLuint shader_id;
  int expr_id;
} Plot;

#define VECTOR_H Plot
#include "../util/vector.h"

typedef struct GraphingTab {
  Mesh square_mesh;

  double last_mouse_x, last_mouse_y;
  bool is_dragging;
  PlotCamera camera;

  vec_ui_expr expressions;

  struct nk_image icons[ICONS_COUNT];

  int prev_fb_width, prev_fb_height;

  Framebuffer read_framebuffer;
  Framebuffer write_framebuffer;

  Shader common_vert;
  GlProgram grid_shader;
  GlProgram post_proc_shader;

  str_t plot_exprs_base;
  vec_NamedShader shaders_pool;
  vec_Plot plots;
} GraphingTab;

GraphingTab* graphing_tab_create(int screen_w, int screen_h);
GLuint load_shader(const char* frag_path, const char* vert_path);
void graphing_tab_resize(GraphingTab* this, int screen_w, int screen_h);

void graphing_tab_free(GraphingTab*);
void graphing_tab_add_shader(GraphingTab*, str_t name, GlProgram shader);
GLuint graphing_tab_get_shader(GraphingTab*, const char* name);
void graphing_tab_update(GraphingTab* this);
void graphing_tab_update_calc(GraphingTab* this);
void graphing_tab_draw(GraphingTab* this, struct nk_context* ctx,
                       GLFWwindow* window);

void graphing_tab_on_scroll(GraphingTab* this, double x, double y);
void graphing_tab_on_mouse_move(GraphingTab* this, double x, double y);
void graphing_tab_on_mouse_click(GraphingTab* this, int button, int action,
                                 int mods);

void ui_expr_update(struct GraphingTab* gt, ui_expr_t* this);
#endif  // SRC_CODE_UI_GRAPHING_TAB_H_