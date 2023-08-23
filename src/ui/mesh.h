#ifndef SRC_UTIL_MESH_H_
#define SRC_UTIL_MESH_H_

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

typedef struct Mesh {
  GLuint vao, vbo, ebo;
  GLenum index_type;
  int indices_count;
} Mesh;

Mesh mesh_create();
void mesh_delete(Mesh);

void mesh_bind(Mesh);
void mesh_unbind();

void mesh_set_vertex_data(Mesh*, void* data, int length, GLenum usage);
void mesh_set_indices(Mesh*, void* data, int length, int indices_count,
                      GLenum usage, GLenum index_type);
void mesh_set_indices_int_tuples(Mesh*, int* data, int len, GLenum usage);

void mesh_draw(Mesh);

typedef struct MeshAttrib {
  int elements_count;
  int element_size;
  GLenum element_type;
} MeshAttrib;
void mesh_bind_consecutive_attribs(Mesh, int start_id, MeshAttrib* attribs,
                                   int count);

#endif  // SRC_UTIL_MESH_H_