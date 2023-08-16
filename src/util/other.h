#ifndef SRC_UTIL_OTHER_H_
#define SRC_UTIL_OTHER_H_

#include <stdbool.h>

#include "../full_nuklear.h"
#include "../util/better_string.h"

struct nk_image load_nk_icon(const char* path);
void delete_nk_icon(struct nk_image img);

bool str_slice_eq_ccp(StrSlice slice, const char* string);

#endif  // SRC_UTIL_OTHER_H_