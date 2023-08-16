
#include "common_vecs.h"

#include "allocator.h"

#define VECTOR_NO_HEADERS
#define VECTOR_IMPLEMENTATION
#define VECTOR_ITEM_TYPE char
#include "../util/vector.h"
#undef VECTOR_ITEM_TYPE

#define VECTOR_IMPLEMENTATION
#define VECTOR_ITEM_TYPE void_ptr
#include "../util/vector.h"
#undef VECTOR_ITEM_TYPE

//.