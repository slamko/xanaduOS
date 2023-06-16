#include "mem/paging.h"
#include "drivers/fb.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "drivers/int.h"
#include "lib/kernel.h"
#include "lib/slibc.h"

#define VADDR 0xC0000000

enum {
    PRESENT              = (1 << 0),
    R_W                  = (1 << 1),
    USER_SUPERVISOR      = (1 << 2),
    PWT                  = (1 << 3),
    PCD                  = (1 << 4),
    ACCESSED             = (1 << 5),
    DIRTY                = (1 << 6),
    PS                   = (1 << 7),
};

struct kalloc_table {
    bool used;
    unsigned int id;
};

struct virt_addr {
    uintptr_t pde : 10;
    uintptr_t pte : 10;
    uintptr_t p_offset : 12;
} __attribute__((packed));

void load_page_dir(uintptr_t dir);
void enable_paging(void);
void print_cr0(void);

uintptr_t page_dir[1024] __attribute__((aligned(4096)));
uintptr_t kernel_page_table[4096] __attribute__((aligned(4096)));

struct kalloc_table kalloc_dir[1024];

extern uintptr_t _kernel_end;
static uintptr_t kernel_end_addr;
static uintptr_t heap_base_addr;

void func(const char *ad) {
    fb_print_num((uintptr_t)ad);
}

void load_all_tables() {
    
    /* for (unsigned int i = 0; i < ; i++) { */
    
    for (unsigned int i = 0; i < 0x400 - 4; i++) {
        ((uintptr_t *)(void *)_kernel_end)[i * 0x1000] =
            (_kernel_end + 0x400000 + (i * 0x1000));
        page_dir[i + 4] = _kernel_end + (i * 0x1000);
    }
}
    
void paging_init() {
    for (unsigned int i = 0; i < ARR_SIZE(page_dir); i++) {
        page_dir[i] |= R_W;
    }

    for (unsigned int i = 0; i < ARR_SIZE(kernel_page_table); i++) {
        kernel_page_table[i] = (i * 0x1000) | PRESENT | USER_SUPERVISOR;
    }

    page_dir[0] = (uintptr_t)&kernel_page_table[1024 * 0] | 0x3;
    page_dir[1] = (uintptr_t)&kernel_page_table[1024 * 1] | 0x3;
    page_dir[2] = (uintptr_t)&kernel_page_table[1024 * 2] | 0x3;
    page_dir[3] = (uintptr_t)&kernel_page_table[1024 * 3] | 0x3;
    
    asm volatile ("mov $_kernel_end, %0" : "=r" (kernel_end_addr));
    heap_base_addr = kernel_end_addr + 0x1000;
    /* fb_print_num(heap_base_addr); */
    
    load_page_dir((uintptr_t)&page_dir);
    /* load_all_tables(); */
    enable_paging();


    klog("Paging enabled\n");
}

static inline int tab_present(uintptr_t descriptor) {
    return descriptor & PRESENT;
}

int map_page(uint16_t pde, uint16_t pte) {
    (*(uintptr_t **)page_dir[pde])[pte] =
            ((0x1000 * pte) + (0x400000 * pde)) | 0x3;

    klog("Map page\n");

    return pte;
}

int alloc_table(uint16_t pde, uint16_t pte) {

    if (!tab_present(page_dir[pde])) {
        uintptr_t table_addr = heap_base_addr + ((pde - 4) * 0x2000);
        uintptr_t **table_addr_ptr = (uintptr_t **)((void *)table_addr);

        page_dir[pde] = table_addr;
        *table_addr_ptr = (uintptr_t *)(table_addr + 0x1000);
        fb_print_num((uintptr_t)(*(uintptr_t **)page_dir[pde]));
        fb_print_num((uintptr_t)*table_addr_ptr);

        for (unsigned int i = 0; i < 1024; i++) {
            map_page(pde, i);
        }

        klog("Alloc table\n"); 
        page_dir[pde] |= 0x3;
        return pde;
    } else if (!tab_present(((uintptr_t *)page_dir[pde])[pte])) {
        map_page(pde, pte);
    }

    return -1;
}


void *kalloc(size_t siz) {
    for (unsigned int i = 0; i < ARR_SIZE(page_dir); i++) {
        unsigned int pd = i + 4;

        alloc_table(pd, 0);
        return (void *)(pd << 22);
    }

    return NULL;
}

void page_fault(struct isr_handler_args args) {
    uintptr_t fault_addr;
    uint16_t pde;
    uint16_t pte;

    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));

    pde = fault_addr >> 22;
    pte = (fault_addr >> 12) & 0x3ff;

    fb_print_num(page_dir[pde] & PRESENT);
    /* fb_print_num(((uintptr_t)page_dir[pde])); */

    /* struct virt_addr fault_virt_addr = *(struct virt_addr *)&fault_addr; */

    if (!(page_dir[pde] & PRESENT)) {
        alloc_table(pde, pte);
        /* fb_print_num((*(uintptr_t **)page_dir[pde])[pte]); */
        /* pde = kmalloc(sizeof(kmalloc_page_table)); */
    }
    
    /* fb_print_num(args.error); */
}

