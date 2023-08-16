#ifndef INCLUDE_SHADER_LOADER_H_
#define INCLUDE_SHADER_LOADER_H_

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

//pub unsafe fn sl_load_program(files: *const *const c_char, shader_types: *const GLenum, length: i32) -> GLuint
extern GLuint sl_load_program(const char** files, const GLenum* shader_types, int32_t length);

// pub unsafe extern "C" fn sl_init_opengl(window: *mut glfw::ffi::GLFWwindow, loader: *const fn(*const std::ffi::c_char) -> *mut std::ffi::c_void)
extern void sl_init_opengl(void* (*loader)(const char*));

//pub fn current_time_secs() -> f64;
extern double current_time_secs();

#endif //INCLUDE_SHADER_LOADER_H_