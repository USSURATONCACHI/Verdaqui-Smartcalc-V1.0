#include "classic_tab.h"

#include <float.h>
#include "../util/allocator.h"

#include "../calculator/calc_backend.h"

#define EDIT_FLAGS NK_EDIT_SIMPLE | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD


ClassicTab classic_tab_create() {
    ClassicTab res =  {
        .x = 0.0,
        .y = 0.0,
        .last_err_message = str_literal(""),
    };
    nk_textedit_init_default(&res.expr_text);
    return res;
}
void classic_tab_free(ClassicTab tab) {
    nk_textedit_free(&tab.expr_text);
};

static void default_button(ClassicTab* this, struct nk_context* ctx, const char* text);

void classic_tab_draw(ClassicTab* this, struct nk_context* ctx,
                      GLFWwindow* window) {
    // .
    nk_layout_row_dynamic(ctx, 30, 1);
    nk_label(ctx, "Calculator", NK_TEXT_ALIGN_LEFT);
    nk_property_double(ctx, "X value", -DBL_MAX, &this->x, DBL_MAX, 0.0, 1.0);
    nk_property_double(ctx, "Y value", -DBL_MAX, &this->y, DBL_MAX, 0.0, 1.0);
    
    nk_edit_buffer(ctx, EDIT_FLAGS, &this->expr_text, nk_filter_default);
    nk_label(ctx, this->last_err_message.string, NK_TEXT_ALIGN_LEFT);

    nk_layout_row_dynamic(ctx, 50, 6);
    const char* const buttons_1[] = {
        "1", "2", "3", "+", "-", "*", /* . */
        "4", "5", "6", "/", "^", "mod", /* . */
        "7", "8", "9", "(", ")", ".", /* . */
        "C", "0","CA", "x", "y", /* . */
    };

    for (int i = 0; i < (int)LEN(buttons_1); i++)
        default_button(this, ctx, buttons_1[i]);
    
    if (nk_button_label(ctx, "=")) {
        const char* buffer = nk_str_get_const(&this->expr_text.string);
        int len = nk_str_len(&this->expr_text.string);

        str_t owned = str_owned("%.*s", len, buffer);
        ExprValueResult res = calc_calculate_expr(owned.string, this->x, this->y);
        str_free(owned);

        str_t result;
        if (res.is_ok) {
            str_t res_text =  str_owned("%$expr_value", res.ok);
            nk_str_clear(&this->expr_text.string);
            nk_str_append_str_char(&this->expr_text.string, res_text.string);
            str_free(res_text);
            
            this->last_err_message = str_literal("");
            expr_value_free(res.ok);
        } else {
            this->last_err_message = res.err_text;
        }
    }

    nk_spacer(ctx);
    nk_layout_row_dynamic(ctx, 50, 6);
    const char* const buttons_2[] = {
        "sin", "cos", "tan", "asin", "acos", "atan"
    };

    for (int i = 0; i < (int)LEN(buttons_2); i++)
        default_button(this, ctx, buttons_2[i]);
}


static void default_button(ClassicTab* this, struct nk_context* ctx, const char* text) {
    if (nk_button_label(ctx, text)) {
        int len = nk_str_len(&this->expr_text.string);

        if (strcmp(text, "C") is 0) {
            nk_str_delete_chars(&this->expr_text.string, len - 1, 1);
        } else if (strcmp(text, "CA") is 0) {
            nk_str_delete_chars(&this->expr_text.string, 0, len);
        } else {
            char last_char = len > 0 ? nk_str_get_const(&this->expr_text.string)[len - 1] : '\0';
            bool is_space = last_char is ' ';
            bool is_num = (last_char >= '0' and last_char <= '9') or last_char is '.';
            bool current_is_num = (text[0] >= '0' and text[0] <= '9') or text[0] is '.';

            if (not is_space and not (is_num and current_is_num))
                nk_str_append_str_char(&this->expr_text.string, " ");
            
            nk_str_append_str_char(&this->expr_text.string, text);
        }
    }
}

void classic_tab_update(ClassicTab* this) {
    unused(this);
}

void classic_tab_on_scroll(ClassicTab* this, double x, double y) {
    unused(this);
    unused(x);
    unused(y);
}

void classic_tab_on_mouse_move(ClassicTab* this, double x, double y) {
    unused(this);
    unused(x);
    unused(y);
}

void classic_tab_on_mouse_click(ClassicTab* this, int button, int action, int mods) {
    unused(this);
    unused(button);
    unused(action);
    unused(mods);
}