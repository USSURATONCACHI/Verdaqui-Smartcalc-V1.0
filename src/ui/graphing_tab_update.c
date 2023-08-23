#include <stdio.h>
#include <stdlib.h>

#include "../calculator/calc_backend.h"
#include "../glsl_compiler/glsl_compiler.h"
#include "../util/allocator.h"
#include "graphing_tab.h"
#include "ui_expr.h"

static str_t copy_from_nk_textedit(struct nk_text_edit* textedit) {
  const char* text = nk_str_get_const(&textedit->string);
  int length = nk_str_len(&textedit->string);
  return str_owned("%.*s", length, text);
}

void graphing_tab_update_calc(GraphingTab* this) {
  CalcBackend calc = calc_backend_create();

  vec_Plot_free(this->plots);
  this->plots = vec_Plot_create();

  // 1. Delete old shaders. (TODO: add shaders caching to not recompile those
  // every time)
  GlslContext glsl = glsl_context_create();

  for (int i = 0; i < this->expressions.length; i++) {
    ui_expr* item = &this->expressions.data[i];
    bool are_only_spaces = true;
    const char* nk_buffer = nk_str_get_const(&item->textedit.string);
    int length = nk_str_len(&item->textedit.string);

    for (int i = 0; i < length and are_only_spaces; i++)
      if (nk_buffer[i] is_not ' ') are_only_spaces = false;

    if (are_only_spaces) {
      str_free(item->descr_text);
      item->descr_text = str_literal("");
      continue;
    }

    str_t buffer = copy_from_nk_textedit(&item->textedit);

    // ADD ITEM
    // debugln("Adding expr: %s", buffer.string);
    // debug_push();

    int prev_length = calc.expressions.length;
    str_t message = calc_backend_add_expr(&calc, buffer.string);

    // debug_pop();
    //    debugln("Adding done (%s).", message.string);
    str_free(buffer);
    str_free(item->descr_text);
    item->descr_text = message;

    CalcExpr* last_expr = calc_backend_last_expr(&calc);

    if (last_expr and prev_length != calc.expressions.length) {
      // debugln("Successfully CalcExpr '%$calc_expr' of type %s",
      // &last_expr->expression, calc_expr_type_text(last_expr->type));

      if (last_expr->type is CALC_EXPR_PLOT) {
        debugln("Adding a plot");
        // debugln("0. Compile to glsl");

        ExprContext ctx = calc_backend_get_context(&calc);
        vec_str_t used_args = vec_str_t_create();
        StrResult code = glsl_compile_expression(
            ctx, &glsl, &last_expr->expression, &used_args);
        vec_str_t_free(used_args);

        if (code.is_ok) {
          // debugln("1. Combine into full shader code");
          StringStream string_stream = string_stream_create();
          OutStream stream = string_stream_stream(&string_stream);

          outstream_puts(this->plot_exprs_base.string, stream);
          outstream_puts("\n", stream);
          glsl_context_print_all_functions(&glsl, stream);

          outstream_puts("\n\nfloat function(vec2 pos, vec2 step) {\n return ",
                         stream);
          outstream_puts(code.data.string, stream);
          outstream_puts(";\n}\n", stream);

          str_free(code.data);
          str_t shader_src = string_stream_to_str_t(string_stream);

          // debugln("2. Check if already exists");
          GLuint shader = graphing_tab_get_shader(this, shader_src.string);
          if (shader) {
            str_free(shader_src);
          } else {
            // debugln("3. Compile and add the shader");

            Shader sh_compiled =
                shader_from_source(GL_FRAGMENT_SHADER, shader_src.string);
            GlProgram pr_compiled =
                gl_program_from_2_shaders(&this->common_vert, &sh_compiled);
            shader_free(sh_compiled);

            debugln("Compiled shader for expr: '%$expr'",
                    last_expr->expression);
            graphing_tab_add_shader(this, shader_src, pr_compiled);
            shader = pr_compiled.program;
          }
          vec_Plot_push(&this->plots,
                        (Plot){.expr_id = i, .shader_id = shader});
        } else {
          debugln("Failed to compile to GLSL cuz: %s", code.data.string);
          str_free(item->descr_text);
          item->descr_text = code.data;
        }
      }
    }
  }

  glsl_context_free(glsl);
  calc_backend_free(calc);
}

void ui_expr_update(GraphingTab* gt, ui_expr_t* this) {
  bool unfocused =
      this->prev_active != this->textedit.active and not this->textedit.active;

  bool buf_changed =
      this->prev_buffer != nk_str_get_const(&this->textedit.string);
  bool len_changed = this->prev_length != nk_str_len(&this->textedit.string);

  if (unfocused or buf_changed or len_changed) {
    // debugc("\n");
    // debugln("Dump before...");
    // my_allocator_dump_short();
    // debugc("\n");

    graphing_tab_update_calc(gt);

    // debugc("\n");
    // debugln("Dump after...");
    // my_allocator_dump_short();
  }

  this->prev_length = nk_str_len(&this->textedit.string);
  this->prev_buffer = nk_str_get_const(&this->textedit.string);
  this->prev_active = this->textedit.active;
}

/*

      if (last_expr->type is CALC_EXPR_PLOT and drawn_plots_count is 0) {
        drawn_plots_count++;
        debugln("Gonna try to convert in into GLSL code...");
        debug_push();

        ExprContext ctx = calc_backend_get_context(&this->calc);
        vec_str_t used_args = vec_str_t_create();
        StrResult res = glsl_compile_expression(
            ctx, &glsl, &last_expr->expression, &used_args);
        vec_str_t_free(used_args);

        debug_pop();
        debugln("Conversion done");

        if (res.is_ok) {
          debugln("Result - OK: %s", res.data.string);
          // Write to file
          FILE* file = fopen("assets/shaders/cache/function.glsl", "w+");
          assert_m(file);
          OutStream os = outstream_from_file(file);
          glsl_context_print_all_functions(&glsl, os);
          x_sprintf(
              os, "\n\nfloat function(vec2 pos, vec2 step) {\nreturn %s;\n}\n",
              res.data.string);
          fclose(file);

          // Reload shader
          glDeleteProgram(this->plot_shader);
          this->plot_shader = load_shader();
          assert_m(this->plot_shader);
          str_free(res.data);
        } else {
          debugln("Result - FAIL: %s", res.data.string);
          str_free(item->descr_text);
          item->descr_text = res.data;
        }
      }
*/