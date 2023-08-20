#ifndef SRC_UI_FRAMEBUFFER_H_
#define SRC_UI_FRAMEBUFFER_H_

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

typedef struct Framebuffer {
  GLuint framebuffer;
  GLuint color_texture;
} Framebuffer;

Framebuffer framebuffer_create(int width, int height, int samples);
void framebuffer_free(Framebuffer f);

void framebuffer_resize(Framebuffer* fb, int width, int height, int samples);

#endif  // SRC_UI_FRAMEBUFFER_H_