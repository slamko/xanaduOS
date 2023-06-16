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

#define KERNEL_INIT_PT_COUNT 4
#define PT_SIZE 1024
#define PAGE_SIZE 4096

static uintptr_t page_dir[1024] __attribute__((aligned(PAGE_SIZE)));

static uintptr_t kernel_page_table[KERNEL_INIT_PT_COUNT * 0x400]
    __attribute__((aligned(PAGE_SIZE)));

extern uintptr_t _kernel_end;
static uintptr_t kernel_end_addr __attribute__((aligned(PAGE_SIZE)));
static uintptr_t heap_base_addr __attribute__((aligned(PAGE_SIZE)));
  
void paging_init() {
    for (unsigned int i = 0; i < ARR_SIZE(page_dir); i++) {
        page_dir[i] |= R_W;
    }

    for (unsigned int i = 0; i < ARR_SIZE(kernel_page_table); i++) {
        kernel_page_table[i] = (i * 0x1000) | PRESENT | USER_SUPERVISOR;
    }

    for (unsigned int i = 0; i < KERNEL_INIT_PT_COUNT; i++) {
        page_dir[i] = (uintptr_t)&kernel_page_table[PT_SIZE * i] | 0x3;
    }
    
    asm volatile ("mov $_kernel_end, %0" : "=r" (kernel_end_addr));
    heap_base_addr = kernel_end_addr + 0x1000;

    load_page_dir((uintptr_t)&page_dir);
    enable_paging();

    klog("Paging enabled\n");
}

static inline int tab_present(uintptr_t descriptor) {
    return descriptor & PRESENT;
}

static inline uintptr_t get_page_addr(uint16_t pde, uint16_t pte) {
    return (pde * 0x400000) + (pte * 0x1000);
}

int map_page(uintptr_t *pt_addr, uint16_t pde, uint16_t pte) {
    pt_addr[pte] = get_page_addr(pde, pte) | 0x3;

    return pte;
}

int pt_present(uint16_t pde) {
    return tab_present(page_dir[pde]);
}

int page_present(uint16_t pde, uint16_t pte) {
    return tab_present(((uintptr_t *)page_dir[pde])[pte]);
}

int map_pt(uint16_t pde) {
    uintptr_t table_addr = heap_base_addr + (pde * 0x1000);

    for (unsigned int i = 0; i < 1024; i++) {
        map_page((uintptr_t *)table_addr, pde, i);
    }

    klog("Alloc page table\n"); 
    page_dir[pde] = table_addr | 0x3;

    return pde;
 }

int page_fault_handle(uint16_t pde, uint16_t pte) {
    if (!pt_present(pde)) {
        map_pt(pde);
    } else if (!page_present(pde, pte)) {
        map_page((uintptr_t *)page_dir[pde], pde, pte);
    }

    return 1;
}

void *kalloc(size_t siz) {
    for (unsigned int i = 0; i < ARR_SIZE(page_dir); i++) {
        unsigned int pd = i + 4;

        return (void *)(pd << 22);
    }

    return NULL;
}

void page_fault(struct isr_handler_args args) {
    uintptr_t fault_addr;
    uint16_t pde;
    uint16_t pte;

    klog_warn("Page fault\n");
    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));

    pde = fault_addr >> 22;
    pte = (fault_addr >> 12) & 0x3ff;

    page_fault_handle(pde, pte);
    
    fb_print_num(args.error);
}

