#include "allocator.h"

#include <stdlib.h>

#include "prettify_c.h"

#undef VECTOR_MALLOC_FN
#undef VECTOR_FREE_FN
#undef VECTOR_REALLOC_FN
#define VECTOR_C MemRegion
#include "vector.h"

// static vec_MemRegion regions = {.data = null, .capacity = 0, .length = 0};

void my_allocator_free() {
  return;

  /*
    for (int i = 0; i < regions.length; i++) {
      debugln("Non freed allocation at %p for %d bytes. Freeing forcibly...",
              regions.data[i].ptr, (int)regions.data[i].size);
      free(regions.data[i].ptr);
    }
    vec_MemRegion_free(regions);
    regions = vec_MemRegion_create();
  */
}

void* my_malloc(size_t size) {
  return malloc(size);

  /*
    MemRegion r = {
        .ptr = malloc(size),
        .size = size,
    };
    vec_MemRegion_push(&regions, r);
    // debugln("Alloc for %ld at %p", (long)r.size, r.ptr);
    return r.ptr;
  */
}

void* my_realloc(void* mem, size_t size) {
  return realloc(mem, size);

  /*
    if (mem) {
      for (int i = 0; i < regions.length; i++) {
        if (regions.data[i].ptr == mem) {
          void* new_mem = realloc(mem, size);
          // debugln("Realloc %p with %ld bytes into %p with %ld bytes",
          // regions.data[i].ptr, (long)regions.data[i].size, new_mem,
          // (long)size);
          regions.data[i].ptr = new_mem;
          regions.data[i].size = size;
          return new_mem;
        }
      }

      panic("Unknown realloc pointer: %p", mem);
    } else {
      return my_malloc(size);
    }
  */
}
void my_free(void* mem) {
  return free(mem);

  /*
    if (not mem) return;

    for (int i = 0; i < regions.length; i++) {
      if (regions.data[i].ptr == mem) {
        // debugln("Free of %p", mem);
        free(mem);
        vec_MemRegion_delete_fast(&regions, i);
        return;
      }
    }

    debugln("Unknown free pointer: %p", mem);
    free(mem);
  */
}

void my_allocator_dump() {
  return;

  /*
    debugln("Allocator has %d active memory regions (cap %d):", regions.length,
            regions.capacity);

    for (int i = 0; i < regions.length; i++) {
      debugc("Ptr: %p | Size: %d\n", regions.data[i].ptr,
             (int)regions.data[i].size);
    }

    debugln("Allocator dump done");
  */
}

void my_allocator_dump_short() {
  return;
  /*
    size_t sum_size = 0;

    for (int i = 0; i < regions.length; i++) sum_size += regions.data[i].size;

    debugln(
        "Allocator has %d(/%d) active memory regions for the total size of %ld",
        regions.length, regions.capacity, (long)sum_size);
  */
}