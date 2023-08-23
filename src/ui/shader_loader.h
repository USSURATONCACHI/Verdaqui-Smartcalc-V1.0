#ifndef SRC_UI_SHADER_LOADER_H_
#define SRC_UI_SHADER_LOADER_H_

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

typedef struct Shader {
  GLuint shader;
} Shader;

Shader shader_from_source(GLenum type, const char* source);
Shader shader_from_file(GLenum type, const char* filepath);
void shader_free(Shader);

typedef struct GlProgram {
  GLuint program;
} GlProgram;

GlProgram gl_program_from_2_shaders(const Shader* a, const Shader* b);
GlProgram gl_program_from_sh_and_f(const Shader* a, GLenum b_type,
                                   const char* b_path);
void gl_program_free(GlProgram);

#endif