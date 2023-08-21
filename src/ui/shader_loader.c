#include <stdbool.h>
#include "../util/better_string.h"
#include "shader_loader.h"
#include "../util/prettify_c.h"

#define LOG_BUF_SIZE (1024 * 512)


Shader shader_from_source(GLenum type, const char* source);
void shader_free(Shader this) {
    glDeleteShader(this.shader);
}

Shader shader_from_source(GLuint type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, null);
    glCompileShader(shader);

    int success = false;
    char info_log[LOG_BUF_SIZE] = "";
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (not success) {
        glGetShaderInfoLog(shader, LOG_BUF_SIZE, null, info_log);
        panic("Shader compilation error: %s", info_log);
    }

    return (Shader) {.shader = shader};
}


GlProgram gl_program_from_2_shaders(const Shader* a, const Shader* b) {
    GLuint program = glCreateProgram();
    glAttachShader(program, a->shader);
    glAttachShader(program, b->shader);
    glLinkProgram(program);
    
    int success = false;
    char info_log[LOG_BUF_SIZE] = "";
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (not success) {
        glGetProgramInfoLog(program, LOG_BUF_SIZE, null, info_log);
        panic("Shader linking error: %s", info_log);
    }

    return (GlProgram) {
        .program = program,
    };
}
void gl_program_free(GlProgram this) {
    glDeleteProgram(this.program);
}


Shader shader_from_file(GLenum type, const char* filepath) {
    str_t src = read_file_to_str(filepath);
    Shader result = shader_from_source(type, src.string);
    str_free(src);
    return result;
}


GlProgram gl_program_from_sh_and_f(const Shader* a, GLenum b_type, const char* b_path) {
    Shader b = shader_from_file(b_type, b_path);
    GlProgram result = gl_program_from_2_shaders(a, &b);
    shader_free(b);
    return result;
}