#ifndef SRC_UTIL_ALLOCATOR_H_
#define SRC_UTIL_ALLOCATOR_H_

#include <stdint.h>
#include <stdlib.h>

typedef struct MemRegion {
  void* ptr;
  size_t size;
} MemRegion;

#define VECTOR_H MemRegion
#include "vector.h"

#undef VECTOR_MALLOC_FN
#undef VECTOR_REALLOC_FN
#undef VECTOR_FREE_FN

void* my_malloc(size_t size);
void* my_realloc(void* mem, size_t size);
void my_free(void* mem);

void my_allocator_dump();
void my_allocator_free();
void my_allocator_dump_short();

#define MALLOC my_malloc
#define REALLOC my_realloc
#define FREE my_free
#define VECTOR_MALLOC_FN my_malloc
#define VECTOR_REALLOC_FN my_realloc
#define VECTOR_FREE_FN my_free

#endif  // SRC_UTIL_ALLOCATOR_H_