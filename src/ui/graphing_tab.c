#include "graphing_tab.h"

#include <float.h>
#include <math.h>
#include <shader_loader.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../calculator/glsl_renderer.h"
#include "../util/allocator.h"
#include "../util/other.h"
#include "../util/prettify_c.h"

#define VECTOR_C ui_expr
#define VECTOR_ITEM_DESTRUCTOR ui_expr_free
#include "../util/vector.h"
#undef VECTOR_MALLOC_FN
#undef VECTOR_FREE_FN

#define EDIT_FLAGS NK_EDIT_SIMPLE | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD

#define ZOOM_BASE 1.3
#define ZOOM_SENSITIVITY 2.0

#define SIDEBAR_WIDTH 500

static Mesh create_square_mesh();

GraphingTab* graphing_tab_create() {
  const char* filepaths[] = {"assets/shaders/test_shader.frag",
                             "assets/shaders/test_shader.vert"};
  const GLenum shader_types[] = {GL_FRAGMENT_SHADER, GL_VERTEX_SHADER};

  debugln("Creating graphing tab (%d)...", (int)sizeof(GraphingTab));
  GraphingTab* result = (GraphingTab*)MALLOC(sizeof(GraphingTab));
  assert_alloc(result);

  (*result) = (GraphingTab){
      .plot_shader = sl_load_program(filepaths, shader_types, 2),
      .camera = PlotCamera_new(0.0, 0.0),
      .square_mesh = create_square_mesh(),

      .last_mouse_x = 0,
      .last_mouse_y = 0,
      .is_dragging = false,

      .expressions = vec_ui_expr_create(),
      .calc = calc_backend_create(),
      .icons =
          {
              load_nk_icon("assets/img/home.png"),
              load_nk_icon("assets/img/close.png"),
              load_nk_icon("assets/img/plus.png"),
          },
  };
  // vec_ui_expr_push(&result->expressions,
  //  ui_expr_create("sin(x * 2e) + 4 * e^(sin(4))"));
  vec_ui_expr_push(&result->expressions, ui_expr_create("a = 2x + 3"));
  vec_ui_expr_push(&result->expressions, ui_expr_create("b(a, b) = a^b + y"));
  vec_ui_expr_push(&result->expressions, ui_expr_create("a = b(2, 3)"));
  debugln("Done creating GraphingTab");
  // vec_ui_expr_push(&result->expressions, ui_expr_create("f(b, c) = b[c]"));
  // vec_ui_expr_push(
  // &result->expressions,
  // ui_expr_create("sin cos tan (x * y) = sin cos tan x + sin cos tan y"));

  return result;
}

void graphing_tab_free(GraphingTab* this) {
  debugln("Graphing tab - deleting shader...");
  glDeleteProgram(this->plot_shader);
  debugln("Graphing tab - deleting mesh...");
  mesh_delete(this->square_mesh);
  debugln("Graphing tab - deleting ui expressions...");
  vec_ui_expr_free(this->expressions);
  debugln("Graphing tab - deleting calculator backend...");
  calc_backend_free(this->calc);

  debugln("Graphing tab - deleting icons...");
  for (int i = 0; i < ICONS_COUNT; i++) delete_nk_icon(this->icons[i]);

  debugln("Graphing tab -  this");
  FREE(this);
}

static void draw_plot(GraphingTab* this, GLFWwindow* window);
static float get_zoom(PlotCamera* camera);
static void draw_exprs_ui(GraphingTab* this, struct nk_context* ctx);

void graphing_tab_draw(GraphingTab* this, struct nk_context* ctx,
                       GLFWwindow* window) {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

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
  if (nk_button_image(ctx, this->icons[ICON_PLUS])) {
    vec_ui_expr_push(&this->expressions, ui_expr_create("y=x"));
  }
}

static float get_zoom(PlotCamera* camera) {
  return pow(ZOOM_BASE, PlotCamera_zoom(camera));
}

static void draw_plot(GraphingTab* this, GLFWwindow* window) {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  float zoom = get_zoom(&this->camera);
  Vector2 step = (Vector2){1.0f / zoom, 1.0f / zoom};
  Vector2 pos = PlotCamera_pos(&this->camera);

  Vector2 offset = (Vector2){-(float)width / 2, -(float)height / 2};

  glUseProgram(this->plot_shader);
  int locStep = glGetUniformLocation(this->plot_shader, "camera_step");
  int locStart = glGetUniformLocation(this->plot_shader, "camera_start");
  int locOffset = glGetUniformLocation(this->plot_shader, "pixel_offset");
  int locWinSize = glGetUniformLocation(this->plot_shader, "window_size");

  glUniform2f(locStep, step.x, step.y);
  glUniform2f(locStart, pos.x, pos.y);
  glUniform2f(locOffset, offset.x, offset.y);
  glUniform2f(locWinSize, width, height);

  mesh_bind(this->square_mesh);
  mesh_draw(this->square_mesh);
  mesh_unbind();
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

ui_expr_t ui_expr_create(const char* text) {
  ui_expr_t this = {.color = {.r = 0.8, .g = 0.2, .b = 0.1, .a = 1.0},
                    .prev_active = false,
                    .descr_text = str_literal("Faz balls")};
  nk_textedit_init_default(&this.textedit);
  nk_str_insert_text_char(&this.textedit.string, 0, text, strlen(text));

  this.color.r = 0.8f;
  this.color.g = 0.2f;
  this.color.b = 0.1f;
  this.color.a = 1.0f;
  return this;
}

void ui_expr_free(ui_expr_t this) {
  debugln("UiExpr_free: Freeing textedit...");
  nk_textedit_free(&this.textedit);
  debugln("UiExpr_free: Freeing descr_text...");
  str_free(this.descr_text);
}

void graphing_tab_update(GraphingTab* this) {
  for (int i = 0; i < this->expressions.length; i++)
    ui_expr_update(this, &this->expressions.data[i]);
}

bool is_function(void* ptr, StrSlice text) {
  ptr = ptr;

  const char* const functions[] = {"sin",  "cos",  "tan", "pow", "asin",
                                   "acos", "atan", "log", "ln"};
  const int functions_count = LEN(functions);

  for (int i = 0; i < functions_count; i++)
    if (text.length == (int)strlen(functions[i]) and
        strncmp(functions[i], text.start, text.length) == 0)
      return true;

  return false;
}

void ui_expr_update(GraphingTab* gt, ui_expr_t* this) {
  if (this->prev_active != this->textedit.active and
      not this->textedit.active) {
    debugc("\n");
    debugln("Dump before...");
    my_allocator_dump_short();
    debugc("\n");

    // for (int i = 0; i < 1000; i++)
    graphing_tab_update_calc(gt);

    debugc("\n");
    debugln("Dump after...");
    my_allocator_dump_short();
  }

  this->prev_active = this->textedit.active;
}

static bool is_calc_expr_const(CalcExpr* expr, CalcBackend* calc) {
  switch (expr->type) {
    case CALC_EXPR_ACTION:
      return false;
      break;
    case CALC_EXPR_FUNCTION:
      return calc_backend_is_function_constexpr(calc,
                                                expr->function.name.string);
      break;
    case CALC_EXPR_VARIABLE:
    case CALC_EXPR_PLOT:
      return calc_backend_is_const_expr(calc, &expr->expression);
      break;

    default:
      panic("Unknown CalcExpr type");
  }
}

static str_t copy_from_nk_textedit(struct nk_text_edit* textedit) {
  char* text = nk_str_get(&textedit->string);
  int length = nk_str_len(&textedit->string);

  return str_owned("%.*s", length, text);
}

static str_t message_apply_constness(CalcExpr* last_expr, CalcBackend* calc,
                                     str_t message) {
  bool is_const = is_calc_expr_const(last_expr, calc);
  debugln("IS CONST? %s", is_const ? "true" : "false");

  str_t result;
  if (is_const) {
    if (last_expr->type is CALC_EXPR_FUNCTION) {
      result = str_owned("%s - const", message.string);
    } else {
      debugln("Calculating value of expression...");
      ExprContext ctx = calc_backend_get_context(calc);
      ExprValueResult val;
      val = expr_calculate_val(&last_expr->expression, ctx);
      debugln("Calculated!");

      if (val.is_ok) {
        result = str_raw_owned(expr_value_str_print(&val.ok, null));
        expr_value_free(val.ok);
      } else {
        result = val.err_text;
      }
    }
    str_free(message);
  } else {
    result = message;
  }

  return result;
}

#define debug_exprln(before, expr, ...) \
  {                                     \
    debug(before);                      \
    expr_print((expr), DEBUG_OUT);      \
    debugc(__VA_ARGS__);                \
    debugc("\n");                       \
  }

void graphing_tab_update_calc(GraphingTab* this) {
  calc_backend_free(this->calc);

  this->calc = calc_backend_create();

  GlslContext glsl = glsl_context_create();

  for (int i = 0; i < this->expressions.length; i++) {
    ui_expr* item = &this->expressions.data[i];

    str_t buffer = copy_from_nk_textedit(&item->textedit);

    // debugln("SAKE. Not even gonna try to add. Continuing...");
    // str_free(buffer);
    // continue;

    debugln("DUMP before adding");
    my_allocator_dump();
    debugln("Adding expr: %s", buffer.string);
    str_t message = calc_backend_add_expr(&this->calc, buffer.string);
    debugln("Adding done (%s).", message.string);
    str_free(buffer);
    str_free(item->descr_text);

    CalcExpr* last_expr = calc_backend_last_expr(&this->calc);

    if (last_expr) {
      debug_exprln("Added CalcExpr '", &last_expr->expression,
                   "' of type %d | With message: %s", last_expr->type,
                   message.string);

      if (strncmp(message.string, "Ok", 2) is 0) {
        // Ok
        debugln("Added is 'Ok'");
        item->descr_text =
            message_apply_constness(last_expr, &this->calc, message);

        // Lets try to convert to opengl
        if (last_expr->type is CALC_EXPR_PLOT) {
          debugln("Trying to convert to GLSL code...");

          vec_str_t used_args = vec_str_t_create();
          StrResult code_res = calc_backend_to_glsl_code(
              &this->calc, &glsl, &last_expr->expression, &used_args);
          vec_str_t_free(used_args);

          if (code_res.is_ok) {
            debugln("Success!");
            str_t functions = glsl_context_get_all_functions(&glsl);
            debugln("THE CODE POINTER: %p", code_res.data.string);
            debugln("It uses these functions:%s\n\nTHE CODE:\n%s",
                    functions.string, code_res.data.string);
          } else {
            debugln("Fail: %s", code_res.data.string);
          }
        }
      } else {
        // Error
        debugln("Added is not 'Ok'");
        item->descr_text = message;
      }
    } else {
      // Error
      item->descr_text = message;
    }
    debugln("Final description: %s\n", item->descr_text.string);
  }

  glsl_context_free(glsl);
}