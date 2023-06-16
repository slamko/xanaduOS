#include "mem/paging.h"
#include "drivers/fb.h"
#include <stddef.h>
#include <stdint.h>
#include "drivers/int.h"
#include "lib/slibc.h"

#define VADDR 0xC0000000

enum PAGE_TABLE_ENTRY_FLAGS {
    PRESENT = 0x1,
    R_W = 0x2,
};

void load_page_dir(uintptr_t dir);
void enable_paging(void);
void print_cr0(void);

uintptr_t page_dir[1024] __attribute__((aligned(4096)));

uintptr_t page_table[1024] __attribute__((aligned(4096)));

void paging_init() {
    for (unsigned int i = 0; i < ARR_SIZE(page_dir); i++) {
        page_dir[i] = 0x2;
    }

    for (unsigned int i = 0; i < ARR_SIZE(page_table); i++) {
        page_table[i] = (i * 0x1000) | 0x3;
    }

    page_dir[0] = (uintptr_t)&page_table | 0x3;
    
    load_page_dir((uintptr_t)&page_dir);
    enable_paging();


    fb_print_attrs("Paging enabled", (struct fb_attr) {.non_deletable = 1});
}

void page_fault(struct isr_handler_args args) {
    int fault_addr;

    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));
    fb_print_num(fault_addr);
    
    fb_newline();
    /* fb_print_num(args.error); */
}

