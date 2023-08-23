#include "framebuffer.h"

#include "../util/allocator.h"
#include "../util/prettify_c.h"

Framebuffer framebuffer_create(int width, int height, int samples) {
  unused(samples);
  GLuint fb;
  glGenFramebuffers(1, &fb);
  glBindFramebuffer(GL_FRAMEBUFFER, fb);

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  // glTexImage2DMultisample(GL_TEXTURE_2D, samples, GL_RGBA, width, height,
  // GL_FALSE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, null);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture, 0);
  assert_m(glCheckFramebufferStatus(GL_FRAMEBUFFER) is GL_FRAMEBUFFER_COMPLETE);

  Framebuffer result = {.color_texture = texture, .framebuffer = fb};
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return result;
}
void framebuffer_free(Framebuffer f) {
  glDeleteTextures(1, &f.color_texture);
  glDeleteFramebuffers(1, &f.framebuffer);
}

void framebuffer_resize(Framebuffer* fb, int width, int height, int samples) {
  unused(samples);
  glBindTexture(GL_TEXTURE_2D, fb->color_texture);
  // glTexImage2DMultisample(GL_TEXTURE_2D, samples, GL_RGBA, width, height,
  // GL_FALSE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, null);
  glBindTexture(GL_TEXTURE_2D, 0);
}