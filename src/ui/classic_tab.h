#ifndef UI_CLASSIC_TAB_
#define UI_CLASSIC_TAB_

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "../nuklear_flags.h"
#include "../util/better_string.h"

typedef struct ClassicTab {
    struct nk_text_edit expr_text;
    double x;
    double y;

    str_t last_err_message;
} ClassicTab;

ClassicTab classic_tab_create();
void classic_tab_free(ClassicTab tab);
void classic_tab_draw(ClassicTab* this, struct nk_context* ctx,
                      GLFWwindow* window);
void classic_tab_update(ClassicTab* this);

void classic_tab_on_scroll(ClassicTab* this, double x, double y);
void classic_tab_on_mouse_move(ClassicTab* this, double x, double y);
void classic_tab_on_mouse_click(ClassicTab* this, int button, int action,
                                int mods);

#endif  // UI_CLASSIC_TAB_
