#ifndef SRC_UTIL_COMMON_VECS_H_
#define SRC_UTIL_COMMON_VECS_H_

// vec_char header + implementation
#define VECTOR_ITEM_TYPE char
#include "../util/vector.h"
#undef VECTOR_ITEM_TYPE

// vec_void_ptr header + implementation
typedef void* void_ptr;
#define VECTOR_ITEM_TYPE void_ptr
#include "../util/vector.h"
#undef VECTOR_ITEM_TYPE

// vec_char header + implementation
#define VECTOR_ITEM_TYPE double
#include "../util/vector.h"
#undef VECTOR_ITEM_TYPE

#endif  // SRC_UTIL_COMMON_VECS_H_