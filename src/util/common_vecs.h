#ifndef SRC_UTIL_COMMON_VECS_H_
#define SRC_UTIL_COMMON_VECS_H_

// vec_char header + implementation
#define VECTOR_H char
#include "../util/vector.h"

// vec_void_ptr header + implementation
typedef void* void_ptr;
#define VECTOR_H void_ptr
#include "../util/vector.h"

#define VECTOR_H double
#include "../util/vector.h"

#endif  // SRC_UTIL_COMMON_VECS_H_