#include "mesh.h"

#include "prettify_c.h"

Mesh mesh_create() {
  Mesh result;

  result.indices_count = 0;
  result.index_type = GL_UNSIGNED_INT;

  glGenBuffers(1, &result.vbo);
  glGenBuffers(1, &result.ebo);
  glGenVertexArrays(1, &result.vao);

  mesh_bind(result);
  // mesh_unbind(result);

  return result;
}
void mesh_delete(Mesh this) {
  glDeleteBuffers(1, &this.vbo);
  glDeleteBuffers(1, &this.ebo);
  glDeleteVertexArrays(1, &this.vao);
}

void mesh_bind(Mesh this) {
  glBindVertexArray(this.vao);
  glBindBuffer(GL_ARRAY_BUFFER, this.vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this.ebo);
}
void mesh_unbind() {
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void mesh_set_vertex_data(Mesh* this, void* data, int length, GLenum usage) {
  glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
  glBufferData(GL_ARRAY_BUFFER, length, data, usage);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void mesh_set_indices(Mesh* this, void* data, int length, int indices_count,
                      GLenum usage, GLenum index_type) {
  this->index_type = index_type;
  this->indices_count = indices_count;

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, length, data, usage);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
void mesh_set_indices_int_tuples(Mesh* this, int* data, int len, GLenum usage) {
  mesh_set_indices(this, data, len * sizeof(int), len, usage, GL_UNSIGNED_INT);
}

void mesh_draw(Mesh this) {
  glDrawElements(GL_TRIANGLES, this.indices_count, this.index_type, null);
}

void mesh_bind_consecutive_attribs(Mesh this, int start_id, MeshAttrib* attribs,
                                   int count) {
  mesh_bind(this);

  int stride = 0;
  for (int i = 0; i < count; i++)
    stride += attribs[i].element_size * attribs[i].elements_count;

  void* ptr = null;
  int index = start_id;

  for (int i = 0; i < count; i++) {
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, attribs[i].elements_count,
                          attribs[i].element_type, GL_FALSE, stride, ptr);

    index++;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
    ptr += attribs[i].element_size * attribs[i].elements_count;
#pragma GCC diagnostic pop
  }
}