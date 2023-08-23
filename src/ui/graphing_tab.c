#include "graphing_tab.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../util/allocator.h"
#include "../util/better_io.h"
#include "../util/other.h"
#include "../util/prettify_c.h"

#define VECTOR_C NamedShader
#define VECTOR_ITEM_DESTRUCTOR named_shader_free
#include "../util/vector.h"  // vec_NamedShader

#define VECTOR_C Plot
#include "../util/vector.h"  // vec_Plot

#define EDIT_FLAGS NK_EDIT_SIMPLE | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD

#define ZOOM_BASE 1.3
#define ZOOM_SENSITIVITY 2.0

#define SIDEBAR_WIDTH 500
#define SSAA 2

void named_shader_free(NamedShader ns) {
  str_free(ns.name);
  gl_program_free(ns.shader);
}

static Mesh create_square_mesh();

GraphingTab* graphing_tab_create(int screen_w, int screen_h) {
  debugln("Creating graphing tab (%d)...", (int)sizeof(GraphingTab));
  GraphingTab* result = (GraphingTab*)MALLOC(sizeof(GraphingTab));
  assert_alloc(result);

  Shader common_vert =
      shader_from_file(GL_VERTEX_SHADER, "assets/shaders/common.vert");

  (*result) = (GraphingTab){
      .camera = PlotCamera_new(0.0, 0.0),
      .square_mesh = create_square_mesh(),

      .last_mouse_x = 0,
      .last_mouse_y = 0,
      .is_dragging = false,

      .expressions = vec_ui_expr_create(),
      .icons =
          {
              load_nk_icon("assets/img/home.png"),
              load_nk_icon("assets/img/close.png"),
              load_nk_icon("assets/img/plus.png"),
          },

      .prev_fb_width = screen_w,
      .prev_fb_height = screen_h,
      .read_framebuffer =
          framebuffer_create(screen_w * SSAA, screen_h * SSAA, MULTISAMPLES),
      .write_framebuffer =
          framebuffer_create(screen_w * SSAA, screen_h * SSAA, MULTISAMPLES),

      .common_vert = common_vert,
      .grid_shader = gl_program_from_sh_and_f(&common_vert, GL_FRAGMENT_SHADER,
                                              "assets/shaders/grid.frag"),
      .post_proc_shader =
          gl_program_from_sh_and_f(&common_vert, GL_FRAGMENT_SHADER,
                                   "assets/shaders/post_processing.frag"),
      .plots = vec_Plot_create(),
      .plot_exprs_base = read_file_to_str("assets/shaders/function.frag"),
  };

  FILE* exprs = fopen("assets/cache/exprs.txt", "r");
  while (exprs and not feof(exprs)) {
    char line[1024] = "";
    char* suc = fgets(line, 1024, exprs);

    if (not suc) break;
    for (int i = 0; line[i] != '\0'; i++)
      if (line[i] is '\n') line[i] = '\0';

    debugln("Reading expression '%s'", line);
    vec_ui_expr_push(&result->expressions, ui_expr_create(line));
  }
  if (exprs) fclose(exprs);

  graphing_tab_update_calc(result);
  debugln("Done creating GraphingTab");

  return result;
}

void graphing_tab_resize(GraphingTab* this, int screen_w, int screen_h) {
  framebuffer_resize(&this->read_framebuffer, screen_w * SSAA, screen_h * SSAA,
                     MULTISAMPLES);
  framebuffer_resize(&this->write_framebuffer, screen_w * SSAA, screen_h * SSAA,
                     MULTISAMPLES);
}

void graphing_tab_free(GraphingTab* this) {
  // SAVE
  debugln("Graphing tab - saving expressions");
  FILE* exprs = fopen("assets/cache/exprs.txt", "w");
  OutStream os = outstream_from_file(exprs);
  for (int i = 0; i < this->expressions.length; i++) {
    struct nk_str* str = &this->expressions.data[i].textedit.string;
    const char* text = nk_str_get_const(str);
    int len = nk_str_len(str);

    outstream_put_slice(text, len, os);
    outstream_puts("\n", os);
  }
  fclose(exprs);

  // FREE
  debugln("Graphing tab - freeing...");
  mesh_delete(this->square_mesh);
  vec_ui_expr_free(this->expressions);

  for (int i = 0; i < ICONS_COUNT; i++) delete_nk_icon(this->icons[i]);

  framebuffer_free(this->read_framebuffer);
  framebuffer_free(this->write_framebuffer);

  shader_free(this->common_vert);
  gl_program_free(this->grid_shader);
  gl_program_free(this->post_proc_shader);

  str_free(this->plot_exprs_base);
  vec_NamedShader_free(this->shaders_pool);
  vec_Plot_free(this->plots);

  FREE(this);
  debugln("Graphing tab - freeing done");
}

void graphing_tab_add_shader(GraphingTab* this, str_t name, GlProgram shader) {
  assert_m(graphing_tab_get_shader(this, name.string) is 0);
  if (this->shaders_pool.length >= GRAPHING_MAX_SHADERS) {
    debugln("Warning: shader pool overflow, deleting oldest shader");
    vec_NamedShader_delete_fast(&this->shaders_pool,
                                0);  // Delete the oldest shader
  }
  vec_NamedShader_push(&this->shaders_pool,
                       (NamedShader){.name = name, .shader = shader});
}
GLuint graphing_tab_get_shader(GraphingTab* this, const char* name) {
  for (int i = 0; i < this->shaders_pool.length; i++) {
    if (strcmp(name, this->shaders_pool.data[i].name.string) is 0)
      return this->shaders_pool.data[i].shader.program;
  }
  return 0;
}

static void draw_plot(GraphingTab* this, GLFWwindow* window);
static float get_zoom(PlotCamera* camera);
static void draw_exprs_ui(GraphingTab* this, struct nk_context* ctx);

void graphing_tab_draw(GraphingTab* this, struct nk_context* ctx,
                       GLFWwindow* window) {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  if (width != this->prev_fb_width or height != this->prev_fb_height or false)
    graphing_tab_resize(this, width, height);

  draw_plot(this, window);

  nk_layout_row_dynamic(ctx, 30, 1);
  nk_label(ctx, "Camera", NK_TEXT_ALIGN_LEFT);

  nk_layout_row_static(ctx, 30, 30, 5);
  if (nk_button_image(ctx, this->icons[ICON_HOME])) {
    this->camera.pos = (Vector2){0.0, 0.0};
    this->camera.vel = (Vector2){0.0, 0.0};
    this->camera.zoom = 20.0;
    this->camera.zoom_vel = 0.0;
  }

  nk_layout_row_dynamic(ctx, 30, 1);
  float zoom = get_zoom(&this->camera);
  Vector2 pos = PlotCamera_pos(&this->camera);
  Vector2 pos_start = pos;
  float zoom_exp = PlotCamera_zoom(&this->camera), zoom_exp_start = zoom_exp;

  nk_property_float(ctx, "Pos X", -FLT_MAX, &pos.x, FLT_MAX, 0.0, 1.0 / zoom);
  nk_property_float(ctx, "Pos Y", -FLT_MAX, &pos.y, FLT_MAX, 0.0, 1.0 / zoom);
  nk_property_float(ctx, "Zoom (exp)", -300.0, &zoom_exp, 300.0, 0.0, 0.1);

  if (pos.x != pos_start.x || pos.y != pos_start.y)
    PlotCamera_set_pos(&this->camera, pos);

  if (zoom_exp != zoom_exp_start) PlotCamera_set_zoom(&this->camera, zoom_exp);

  draw_exprs_ui(this, ctx);
}

static void draw_exprs_ui(GraphingTab* this, struct nk_context* ctx) {
  nk_layout_row_dynamic(ctx, 30, 1);
  nk_label(ctx, "Expressions", NK_TEXT_ALIGN_LEFT);
  for (int i = 0; i < this->expressions.length; i++) {
    ui_expr_t* expr_i = &this->expressions.data[i];
    nk_layout_row_begin(ctx, NK_STATIC, 25, 3);

    nk_layout_row_push(ctx, 25);
    if (nk_button_image(ctx, this->icons[ICON_CROSS])) {
      vec_ui_expr_delete_order(&this->expressions, i--);
      graphing_tab_update_calc(this);
      continue;
    }

    nk_layout_row_push(ctx, 25);
    if (nk_combo_begin_color(ctx, nk_rgb_cf(expr_i->color),
                             nk_vec2(SIDEBAR_WIDTH, 400))) {
      nk_layout_row_dynamic(ctx, 120, 1);
      expr_i->color = nk_color_picker(ctx, expr_i->color, NK_RGBA);
      nk_layout_row_dynamic(ctx, 25, 1);
      expr_i->color.r =
          nk_propertyf(ctx, "#R:", 0, expr_i->color.r, 1.0f, 0.01f, 0.005f);
      expr_i->color.g =
          nk_propertyf(ctx, "#G:", 0, expr_i->color.g, 1.0f, 0.01f, 0.005f);
      expr_i->color.b =
          nk_propertyf(ctx, "#B:", 0, expr_i->color.b, 1.0f, 0.01f, 0.005f);
      expr_i->color.a =
          nk_propertyf(ctx, "#A:", 0, expr_i->color.a, 1.0f, 0.01f, 0.005f);
      nk_combo_end(ctx);
    }

    nk_layout_row_push(ctx, SIDEBAR_WIDTH - 80);
    nk_edit_buffer(ctx, EDIT_FLAGS, &expr_i->textedit, nk_filter_default);

    // nk_layout_row_end(ctx);
    // nk_layout_row_begin(ctx, NK_STATIC, 25, 3);

    // if (not expr_i->descr_text)
    nk_layout_row_push(ctx, SIDEBAR_WIDTH);
    nk_text(ctx, expr_i->descr_text.string, strlen(expr_i->descr_text.string),
            NK_TEXT_ALIGN_LEFT);
    // nk_text_wrap(ctx, expr_i->descr_text.string,
    // strlen(expr_i->descr_text.string)); nk_layout_row_push(ctx,
    // SIDEBAR_WIDTH);

    nk_layout_row_end(ctx);
  }

  nk_layout_row_static(ctx, 25, 25, 1);
  if (this->expressions.length < (GRAPHING_MAX_SHADERS / 2) and
      nk_button_image(ctx, this->icons[ICON_PLUS])) {
    vec_ui_expr_push(&this->expressions, ui_expr_create(""));
    graphing_tab_update_calc(this);
  }
}

static float get_zoom(PlotCamera* camera) {
  return pow(ZOOM_BASE, PlotCamera_zoom(camera));
}

static void bind_uniforms(GraphingTab* this, GLFWwindow* window,
                          GLuint program);
static void bind_framebuffers(GraphingTab* this, GLuint program);
static void swap_framebuffers(GraphingTab* this);

static void swap_bind_bind(GraphingTab* this, GLFWwindow* window,
                           GLuint program);

static void draw_plot(GraphingTab* this, GLFWwindow* window) {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  mesh_bind(this->square_mesh);
  glViewport(0, 0, width * SSAA, height * SSAA);

  // 1. Grid or background
  swap_bind_bind(this, window, this->grid_shader.program);  // Шейдер сетки
  mesh_draw(this->square_mesh);  // Рисуем на весь экран

  // 2. All the plots
  for (int i = 0; i < this->plots.length; i++) {
    Plot plot = this->plots.data[i];
    swap_bind_bind(this, window, plot.shader_id);  // Шейдер графика
    int loc = glGetUniformLocation(plot.shader_id, "u_color");

    struct nk_colorf color = this->expressions.data[plot.expr_id].color;
    glUniform4f(loc, color.r, color.g, color.b,
                color.a);  // Отправляем цвет в шейдер (в униформу u_color)
    mesh_draw(this->square_mesh);  //  Рисуем на весь экран
  }

  // 3. Post processing
  swap_bind_bind(this, window,
                 this->post_proc_shader.program);  // Финальный шейдер
  glBindFramebuffer(GL_FRAMEBUFFER, 0);  // 0 = буффер окна, тоесть на экран
  glViewport(0, 0, width, height);
  mesh_draw(this->square_mesh);  // Рисуем на весь экран

  mesh_unbind();
}
static void swap_bind_bind(GraphingTab* this, GLFWwindow* window,
                           GLuint program) {
  swap_framebuffers(this);
  bind_framebuffers(this, program);
  bind_uniforms(this, window, program);
  glUseProgram(program);
}

static void bind_framebuffers(GraphingTab* this, GLuint program) {
  // debugln("Starting to bind framebuffers..."); debug_push();
  // Bind write FB
  glBindFramebuffer(GL_FRAMEBUFFER, this->write_framebuffer.framebuffer);

  // Bind read FB as texture sampler
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, this->read_framebuffer.color_texture);

  glUseProgram(program);
  int loc = glGetUniformLocation(program, "u_read_texture");

  glUniform1i(loc, 0);  // GL_TEXTURE0 <- 0 is from here
  // debugln("Framebuffers done."); debug_pop();
}

static void swap_framebuffers(GraphingTab* this) {
  SWAP(Framebuffer, this->read_framebuffer, this->write_framebuffer);
}

static void bind_uniforms(GraphingTab* this, GLFWwindow* window,
                          GLuint program) {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  float zoom = get_zoom(&this->camera);
  Vector2 step = (Vector2){1.0f / zoom, 1.0f / zoom};
  Vector2 pos = PlotCamera_pos(&this->camera);

  Vector2 offset = (Vector2){-(float)width / 2, -(float)height / 2};

  glUseProgram(program);
  int locStep = glGetUniformLocation(program, "u_camera_step");
  int locStart = glGetUniformLocation(program, "u_camera_start");
  int locOffset = glGetUniformLocation(program, "u_pixel_offset");
  int locWinSize = glGetUniformLocation(program, "u_window_size");

  glUniform2f(locStep, step.x, step.y);
  glUniform2f(locStart, pos.x, pos.y);
  glUniform2f(locOffset, offset.x, offset.y);
  glUniform2f(locWinSize, width, height);
}

static Mesh create_square_mesh() {
  Mesh mesh = mesh_create();

  MeshAttrib attribs[] = {
      {2, sizeof(float), GL_FLOAT},
      {2, sizeof(float), GL_FLOAT},
  };
  mesh_bind_consecutive_attribs(mesh, 0, attribs,
                                sizeof(attribs) / sizeof(attribs[0]));

  float vertices[] = {
      -1.0, -1.0, 0.0, 0.0, 1.0,  -1.0, 1.0, 0.0,
      1.0,  1.0,  1.0, 1.0, -1.0, 1.0,  0.0, 1.0,
  };
  mesh_set_vertex_data(&mesh, vertices, sizeof(vertices), GL_STATIC_DRAW);

  int indices[] = {
      0, 1, 2, 0, 3, 2,
  };
  mesh_set_indices_int_tuples(
      &mesh, indices, sizeof(indices) / sizeof(indices[0]), GL_STATIC_DRAW);

  return mesh;
}

void graphing_tab_on_scroll(GraphingTab* this, double x, double y) {
  x = x;
  PlotCamera_on_zoom(&this->camera, y);
}

void graphing_tab_on_mouse_move(GraphingTab* this, double x, double y) {
  if (this->is_dragging) {
    float zoom = get_zoom(&this->camera);
    PlotCamera_on_drag(&this->camera,
                       (Vector2){.x = (this->last_mouse_x - x) / zoom,
                                 .y = -(this->last_mouse_y - y) / zoom});
  }

  this->last_mouse_x = x;
  this->last_mouse_y = y;
}

void graphing_tab_on_mouse_click(GraphingTab* this, int button, int action,
                                 int mods) {
  mods = mods;
  if (button is GLFW_MOUSE_BUTTON_LEFT) {
    this->is_dragging =
        action is GLFW_PRESS and this->last_mouse_x > SIDEBAR_WIDTH;

    if (this->is_dragging)
      PlotCamera_on_drag_start(&this->camera);
    else
      PlotCamera_on_drag_end(&this->camera);
  }
}

// ====

void graphing_tab_update(GraphingTab* this) {
  for (int i = 0; i < this->expressions.length; i++)
    ui_expr_update(this, &this->expressions.data[i]);
}
