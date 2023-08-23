#ifndef UI_ICON_LOAD_
#define UI_ICON_LOAD_

#include "../full_nuklear.h"
#include "../util/better_string.h"

struct nk_image load_nk_icon(const char* path);
void delete_nk_icon(struct nk_image img);

#endif  // UI_ICON_LOAD_
