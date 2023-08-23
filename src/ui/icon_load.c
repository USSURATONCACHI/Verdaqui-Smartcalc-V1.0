#include "icon_load.h"

#include <stb_image.h>
#include <stdlib.h>
#include <string.h>

#include "../util/prettify_c.h"

struct nk_image load_nk_icon(const char *path) {
  int x, y, n;
  GLuint tex;
  unsigned char *data = stbi_load(path, &x, &y, &n, 0);
  if (!data) panic("load_nk_icon: Failed to load image: %s", path);

  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  GL_LINEAR_MIPMAP_NEAREST);
  glGetError();  // Just ignore if prev line gives error
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               data);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(data);
  return nk_image_id((int)tex);
}

void delete_nk_icon(struct nk_image img) {
  glDeleteTextures(1, (GLuint *)&img.handle.id);
}
