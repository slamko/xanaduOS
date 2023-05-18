#include "mem/paging.h"
#include "drivers/fb.h"
#include <stddef.h>
#include <stdint.h>

extern void enable_paging();

uintptr_t page_dir[1024] __attribute__((aligned(4096)));

void paging_init() {
    /* enable_paging(); */
    fb_print_attrs("Paging enabled", (struct fb_attr) {.non_deletable = 1});
}
