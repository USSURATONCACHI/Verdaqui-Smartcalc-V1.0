/**
 * This header-only library allows to create template vectors in c.
 * Vectors can hold data with custom destructors. You can set
 * VECTOR_ITEM_DESTRUCTOR to the destructor.
 */

// input macro: VECTOR_ITEM_TYPE - type
// input macro: VECTOR_ITEM_DESTRUCTOR - void function that accepts pointer to
// object, optional input macro: VECTOR_MALLOC_FN - function to allocate memory,
// optional input macro: VECTOR_FREE_FN - function to free memory, optional
// input macro: VECTOR_MAKE_STATIC - define to make all functions static
// input macro: DEBUG
// input macro: VECTOR_NO_HEADERS

// input macto: VECTOR_ITEM_CLONE

#ifdef VECTOR_H
#define VECTOR_ITEM_TYPE VECTOR_H
#undef VECTOR_IMPLEMENTATION
#endif

#ifdef VECTOR_C
#define VECTOR_ITEM_TYPE VECTOR_H
#define VECTOR_NO_HEADERS
#define VECTOR_IMPLEMENTATION
#endif

#ifndef VECTOR_ITEM_TYPE
#warning VECTOR_ITEM_TYPE is undefined, cannot construct vector.
#else
#include <string.h>
#ifndef VECTOR_MALLOC_FN
#include <stdlib.h>
#define VECTOR_MALLOC_FN malloc
#endif

#ifndef VECTOR_FREE_FN
#include <stdlib.h>
#define VECTOR_FREE_FN free
#endif

#ifdef VECTOR_MAKE_STATIC
#define VEC_STATIC_PREFIX static
#else
#define VEC_STATIC_PREFIX
#endif
#include <stddef.h>

#include "prettify_c.h"

#ifdef VECTOR_H
#define VEC_T CONCAT(vec_, VECTOR_H)
#define ITEM_T VECTOR_H
#else
#ifdef VECTOR_C
#define VEC_T CONCAT(vec_, VECTOR_C)
#define ITEM_T VECTOR_C
#else
#define VEC_T CONCAT(vec_, VECTOR_ITEM_TYPE)
#define ITEM_T VECTOR_ITEM_TYPE
#endif
#endif

#ifndef VECTOR_NO_HEADERS
typedef struct {
  VECTOR_ITEM_TYPE* data;
  int length;
  int capacity;
} VEC_T;
#endif

// Names of functions
#define VEC_CREATE CONCAT(VEC_T, _create)
#define VEC_WITH_CAPACITY CONCAT(VEC_T, _with_capacity)
#define VEC_CREATE_COPY CONCAT(VEC_T, _create_copy)
#define VEC_CLONE CONCAT(VEC_T, _clone)
#define VEC_FROM_RAW CONCAT(VEC_T, _from_raw)
#define VEC_PUSH CONCAT(VEC_T, _push)
#define VEC_INSERT CONCAT(VEC_T, _insert)
#define VEC_POPGET CONCAT(VEC_T, _popget)
#define VEC_POPFREE CONCAT(VEC_T, _popfree)
#define VEC_AT CONCAT(VEC_T, _at)
#define VEC_ATREF CONCAT(VEC_T, _atref)
#define VEC_EXTRACT_FAST CONCAT(VEC_T, _extract_fast)
#define VEC_EXTRACT_ORDER CONCAT(VEC_T, _extract_order)
#define VEC_DELETE_FAST CONCAT(VEC_T, _delete_fast)
#define VEC_DELETE_ORDER CONCAT(VEC_T, _delete_order)
#define VEC_FREE CONCAT(VEC_T, _free)

// Function declarations
#ifndef VECTOR_NO_HEADERS
VEC_STATIC_PREFIX VEC_T VEC_CREATE();
VEC_STATIC_PREFIX VEC_T VEC_WITH_CAPACITY(int cap);
VEC_STATIC_PREFIX VEC_T VEC_CREATE_COPY(const ITEM_T* source, int length);
VEC_STATIC_PREFIX VEC_T VEC_CLONE(const VEC_T* source);
VEC_STATIC_PREFIX VEC_T VEC_FROM_RAW(ITEM_T* source, int length);
VEC_STATIC_PREFIX void VEC_PUSH(VEC_T* vec, ITEM_T item);
VEC_STATIC_PREFIX void VEC_INSERT(VEC_T* vec, ITEM_T item, int index);
VEC_STATIC_PREFIX ITEM_T VEC_POPGET(VEC_T* vec);
VEC_STATIC_PREFIX void VEC_POPFREE(VEC_T* vec);
VEC_STATIC_PREFIX ITEM_T VEC_AT(VEC_T* vec, int i);
VEC_STATIC_PREFIX ITEM_T* VEC_ATREF(VEC_T* vec, int i);
VEC_STATIC_PREFIX ITEM_T VEC_EXTRACT_FAST(VEC_T* vec, int i);
VEC_STATIC_PREFIX ITEM_T VEC_EXTRACT_ORDER(VEC_T* vec, int i);
VEC_STATIC_PREFIX void VEC_DELETE_FAST(VEC_T* vec, int i);
VEC_STATIC_PREFIX void VEC_DELETE_ORDER(VEC_T* vec, int i);
VEC_STATIC_PREFIX void VEC_FREE(VEC_T v);
#endif

#ifdef VECTOR_IMPLEMENTATION
// Implementations
VEC_STATIC_PREFIX VEC_T VEC_CREATE() {
  return (VEC_T){
      .data = NULL,
      .length = 0,
      .capacity = 0,
  };
}

VEC_STATIC_PREFIX VEC_T VEC_WITH_CAPACITY(int cap) {
  if (cap is 0)
    return VEC_CREATE();
  else if (cap < 0)
    panic("Cannot create vector with negative capacity (%d)", cap);

  VEC_T result;

  result.data = (ITEM_T*)VECTOR_MALLOC_FN(sizeof(ITEM_T) * cap);
  assert_alloc(result.data);
  result.length = 0;
  result.capacity = cap;

  return result;
}

VEC_STATIC_PREFIX VEC_T VEC_CREATE_COPY(const ITEM_T* source, int length) {
  VEC_T result;

  result.data = (ITEM_T*)VECTOR_MALLOC_FN(sizeof(ITEM_T) * length);
#ifdef VECTOR_ITEM_CLONE
  for (int i = 0; i < length; i++) {
    result.data[i] = VECTOR_ITEM_CLONE(&source[i]);
  }
#else
  memcpy(result.data, source, length * sizeof(ITEM_T));
#endif

  result.length = length;
  result.capacity = length;
  return result;
}

VEC_STATIC_PREFIX VEC_T VEC_CLONE(const VEC_T* source) {
  return VEC_CREATE_COPY(source->data, source->length);
}

VEC_STATIC_PREFIX VEC_T VEC_FROM_RAW(ITEM_T* source, int length) {
  return (VEC_T){
      .data = source,
      .length = length,
      .capacity = length,
  };
}

#ifdef DEBUG
#define CHECK_LEN_ZERO(vec)                                       \
  if ((vec)->length <= 0)                                         \
    panic("%s (at %p) is empty, it's length is %d\n", STR(VEC_T), \
          (void*)(vec), (vec)->length);

#define CHECK_INDEX(vec, index)                                           \
  if (index >= (vec)->length)                                             \
    panic("%s (at %p) index is out of bounds. Index = %d, length = %d\n", \
          STR(VEC_T), (void*)(vec), index, (vec)->length);
#else
#define CHECK_LEN_ZERO(vec) \
  if ((vec)->length <= 0) unreachable();

#define CHECK_INDEX(vec, index) \
  if (index >= (vec)->length) unreachable();
#endif

VEC_STATIC_PREFIX void VEC_PUSH(VEC_T* vec, ITEM_T item) {
  if (vec->length > vec->capacity) {
#ifdef DEBUG
    panic(
        "Absurd situation: vector length (%d) is bigger than it's capacity "
        "(%d)\n",
        vec->length, vec->capacity);
#else
    unreachable();
#endif
  } else if (vec->length == vec->capacity) {
    int new_cap = vec->length * 3 / 2 + 4;
#ifdef VECTOR_REALLOC_FN
    ITEM_T* new_data =
        (ITEM_T*)VECTOR_REALLOC_FN(vec->data, sizeof(ITEM_T) * new_cap);
#else
    ITEM_T* new_data = (ITEM_T*)VECTOR_MALLOC_FN(sizeof(ITEM_T) * new_cap);
    memcpy(new_data, vec->data, sizeof(ITEM_T) * vec->length);
    VECTOR_FREE_FN(vec->data);
#endif

    vec->data = new_data;
    vec->capacity = new_cap;
  }

  vec->data[vec->length] = item;
  vec->length++;
}

VEC_STATIC_PREFIX void VEC_INSERT(VEC_T* vec, ITEM_T item, int index) {
  if (index == vec->length) {
    VEC_PUSH(vec, item);
  } else {
    CHECK_INDEX(vec, index);
    ITEM_T tmp = vec->data[index];
    vec->data[index] = item;
    for (int i = index + 1; i < vec->length; i++) {
      ITEM_T tmp2 = vec->data[i];
      vec->data[i] = tmp;
      tmp = tmp2;
    }
    VEC_PUSH(vec, tmp);
  }
}

VEC_STATIC_PREFIX ITEM_T VEC_POPGET(VEC_T* vec) {
  CHECK_LEN_ZERO(vec);
  vec->length--;
  return vec->data[vec->length];
}

VEC_STATIC_PREFIX void VEC_POPFREE(VEC_T* vec) {
  CHECK_LEN_ZERO(vec);
  vec->length--;

#ifdef VECTOR_ITEM_DESTRUCTOR
  VECTOR_ITEM_DESTRUCTOR(vec->data[vec->length]);
#endif
}

VEC_STATIC_PREFIX ITEM_T VEC_AT(VEC_T* vec, int i) {
  CHECK_INDEX(vec, i);
  return vec->data[i];
}
VEC_STATIC_PREFIX ITEM_T* VEC_ATREF(VEC_T* vec, int i) {
  CHECK_INDEX(vec, i);
  return &vec->data[i];
}

VEC_STATIC_PREFIX ITEM_T VEC_EXTRACT_FAST(VEC_T* vec, int i) {
  CHECK_LEN_ZERO(vec);
  CHECK_INDEX(vec, i);

  ITEM_T item = vec->data[i];
  vec->data[i] = vec->data[vec->length - 1];
  vec->length--;
  return item;
}

VEC_STATIC_PREFIX ITEM_T VEC_EXTRACT_ORDER(VEC_T* vec, int i) {
  CHECK_LEN_ZERO(vec);
  CHECK_INDEX(vec, i);

  ITEM_T item = vec->data[i];
  for (int index = i; index < vec->length - 1; index++)
    vec->data[index] = vec->data[index + 1];
  vec->length--;
  return item;
}

VEC_STATIC_PREFIX void VEC_DELETE_FAST(VEC_T* vec, int i) {
  CHECK_LEN_ZERO(vec);
  CHECK_INDEX(vec, i);

#ifdef VECTOR_ITEM_DESTRUCTOR
  VECTOR_ITEM_DESTRUCTOR(vec->data[i]);
#endif
  vec->data[i] = vec->data[vec->length - 1];
  vec->length--;
}
VEC_STATIC_PREFIX void VEC_DELETE_ORDER(VEC_T* vec, int i) {
  CHECK_LEN_ZERO(vec);
  CHECK_INDEX(vec, i);

#ifdef VECTOR_ITEM_DESTRUCTOR
  VECTOR_ITEM_DESTRUCTOR(vec->data[i]);
#endif

  vec->length--;
  for (int index = i; index < vec->length; index++) {
    vec->data[index] = vec->data[index + 1];
  }
}

VEC_STATIC_PREFIX void VEC_FREE(VEC_T v) {
#ifdef VECTOR_ITEM_DESTRUCTOR
  for (int i = 0; i < v.length; i++) VECTOR_ITEM_DESTRUCTOR(v.data[i]);
#endif

  if (v.data) VECTOR_FREE_FN(v.data);

#ifdef DEBUG
  printf("Deleted " STR(VEC_T) " at %p (length %d)\n", (void*)v, v->length);
#endif
}

#endif

#endif

#undef VECTOR_ITEM_TYPE
#undef VECTOR_ITEM_DESTRUCTOR
#undef VECTOR_ITEM_CLONE
#undef VECTOR_H
#undef VECTOR_C
#undef VEC_T
#undef ITEM_T