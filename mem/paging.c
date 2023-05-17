#include "mem/paging.h"
#include "drivers/fb.h"

extern void enable_paging();

void paging_init() {
    enable_paging();
    fb_print_attrs("Paging enabled", (struct fb_attr) {.non_deletable = 1});
}
