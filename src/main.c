#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "full_nuklear.h"

// Other
#include <shader_loader.h>

#include <nuklear_style.c>

#include "app.h"
#include "util/allocator.h"
#include "util/prettify_c.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#define START_WIN_WIDTH 800
#define START_WIN_HEIGHT 600

// TODO:
// Recompile shader_loader and get_time into minimum binaries

void error_callback(int error, const char* description);
void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action,
                           int mods);

void initialize_all(GLFWwindow** out_window, struct nk_context** nk_ctx,
                    struct nk_glfw* glfw);

static void scroll_callback_in(App* app, GLFWwindow* window, double xoffset,
                               double yoffset);
static void cursor_position_callback_in(App* app, GLFWwindow* window,
                                        double xpos, double ypos);
static void mouse_button_callback_in(App* app, GLFWwindow* window, int button,
                                     int action, int mods);

int main() {
  GLFWwindow* window;
  struct nk_context* ctx;
  struct nk_glfw glfw = {0};
  initialize_all(&window, &ctx, &glfw);

  // ===== Program initialization
  App* app = app_create();
  scroll_callback_in(app, NULL, 0.0, 0.0);
  mouse_button_callback_in(app, NULL, 0, 0, 0);
  cursor_position_callback_in(app, NULL, 0.0, 0.0);

  // ==== Test

  // ===== Main loop
  while (!glfwWindowShouldClose(window)) {
    glfwSwapBuffers(window);
    glfwPollEvents();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    nk_glfw3_new_frame(&glfw);
    app_update(app);
    app_render(app, ctx, window);
    nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER,
                    MAX_ELEMENT_BUFFER);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) printf("GL error: %d\n", err);

    // printf("Now: %lf\n", current_time_secs());
  }

  // Deinitialization
  debugln("Shutting Nuklear down...");
  nk_glfw3_shutdown(&glfw);

  debugln("Freeing the App...");
  app_free(app);

  debugln("Destroying the window...");
  glfwDestroyWindow(window);

  debugln("Terminating GLFW");
  glfwTerminate();

  debugln("Done. Stopping the program");
  my_allocator_dump_short();
  return 0;
}

// ===== GLAD, GLFW, shader_loader and Nuklear initialization
void initialize_all(GLFWwindow** out_window, struct nk_context** nk_ctx,
                    struct nk_glfw* glfw) {
  assert_alloc(out_window);
  assert_alloc(nk_ctx);

  if (!glfwInit()) panic("Failed to initialize GLFW\n");
  glfwSetErrorCallback(error_callback);
  printf("Initialized GLFW\n");

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  (*out_window) = glfwCreateWindow(START_WIN_WIDTH, START_WIN_HEIGHT,
                                   "Verdaqui - Smartcalc C", NULL, NULL);
  if (!(*out_window)) panic("Failed to create window\n");
  printf("Initialized GLFW window\n");

  glfwMakeContextCurrent(*out_window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  printf("Initialized OpenGL context\n");

  glfwSetKeyCallback(*out_window, key_callback);
  glfwSetScrollCallback(*out_window, scroll_callback);
  glfwSetCursorPosCallback(*out_window, cursor_position_callback);
  glfwSetMouseButtonCallback(*out_window, mouse_button_callback);
  glfwSetCharCallback(*out_window, nk_glfw3_char_callback);
  glfwSwapInterval(1);

  sl_init_opengl((GLADloadproc)glfwGetProcAddress);
  printf("Passed OpenGL context to shader_loader rs lib\n");

  // ===== Nuklear initialization
  (*nk_ctx) = nk_glfw3_init(glfw, *out_window, 0);
  printf("Initialized Nuklear lib\n");

  struct nk_font_atlas* atlas;
  nk_glfw3_font_stash_begin(glfw, &atlas);
  struct nk_font* firacode = nk_font_atlas_add_from_file(
      atlas, "assets/fonts/FiraCode-Medium.ttf", 16, 0);
  nk_glfw3_font_stash_end(glfw);
  nk_style_set_font(*nk_ctx, &firacode->handle);
  set_style(*nk_ctx, THEME_RED);
  printf("Added default font to Nuklear\n");
}

// ===== GLFW callbacks
void error_callback(int error, const char* description) {
  fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mods) {
  nk_key_callback(window, key, scancode, action, mods);

  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
  nk_gflw3_scroll_callback(window, xoffset, yoffset);
  scroll_callback_in(null, window, xoffset, yoffset);
}

static void scroll_callback_in(App* app, GLFWwindow* window, double xoffset,
                               double yoffset) {
  static App* current_app;
  if (current_app is null or app is_not null) current_app = app;

  if (current_app is_not null and window is_not null) {
    app_on_scroll(current_app, xoffset, yoffset);
  }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
  cursor_position_callback_in(null, window, xpos, ypos);
}

static void cursor_position_callback_in(App* app, GLFWwindow* window,
                                        double xpos, double ypos) {
  static App* current_app;
  if (current_app is null or app is_not null) current_app = app;

  if (current_app is_not null and window is_not null) {
    app_on_mouse_move(current_app, xpos, ypos);
  }
}

void mouse_button_callback(GLFWwindow* window, int button, int action,
                           int mods) {
  mouse_button_callback_in(null, window, button, action, mods);
  nk_glfw3_mouse_button_callback(window, button, action, mods);
}
static void mouse_button_callback_in(App* app, GLFWwindow* window, int button,
                                     int action, int mods) {
  static App* current_app;
  if (current_app is null or app is_not null) current_app = app;

  if (current_app is_not null and window is_not null) {
    app_on_mouse_click(current_app, button, action, mods);
  }
}