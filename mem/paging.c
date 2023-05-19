#include "mem/paging.h"
#include "drivers/fb.h"
#include <stddef.h>
#include <stdint.h>
#include "lib/slibc.h"

#define VADDR 0xC0000000

enum PAGE_TABLE_ENTRY_FLAGS {
    PRESENT = 0x1,
    R_W = 0x2,
};

extern void load_page_dir(uintptr_t dir);
extern void enable_paging(void);

uintptr_t page_dir[1024] __attribute__((aligned(4096)));

uintptr_t page_table[1024] __attribute__((aligned(4096)));

void *vga_buf;

void paging_init() {

    for (size_t i = 0; i < ARR_SIZE(page_dir); i++) {
        page_dir[i] = 0x2;
    }

    for (size_t i = 0; i < ARR_SIZE(page_table) - 1; i++) {
        page_table[i] = (i * 0x1000) | 0x3; 
    }
    page_table[ARR_SIZE(page_table) - 1] = (0xB8000 | 0x3);
    vga_buf = (void *)page_table[1023];

    page_dir[0] = ((uintptr_t)&page_table - VADDR) | 0x3;
    page_dir[768] = ((uintptr_t)&page_table - VADDR) | 0x3;
    uintptr_t phys_page_dir = (uintptr_t)&page_dir - VADDR;
    
    load_page_dir(phys_page_dir);
    enable_paging();
    /* fb_print_attrs("Paging enabled", (struct fb_attr) {.non_deletable = 1}); */
}
