#include "mem/paging.h"
#include "mem/allocator.h"
#include "mem/frame_allocator.h"
#include "drivers/fb.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/cdefs.h>
#include "drivers/int.h"
#include "lib/kernel.h"
#include "lib/slibc.h"

#define VADDR 0xC0000000

enum {
    PRESENT              = (1 << 0),
    R_W                  = (1 << 1),
    USER                 = (1 << 2),
    PWT                  = (1 << 3),
    PCD                  = (1 << 4),
    ACCESSED             = (1 << 5),
    DIRTY                = (1 << 6),
    PS                   = (1 << 7),
};

struct virt_addr {
    uintptr_t pde : 10;
    uintptr_t pte : 10;
    uintptr_t p_offset : 12;
} __attribute__((packed));

void load_page_dir(uintptr_t dir);
void page_fault(struct isr_handler_args args);
void enable_paging(void);
void print_cr0(void);

#define KERNEL_INIT_PT_COUNT 4

static uintptr_t page_dir[PT_SIZE] __attribute__((aligned(PAGE_SIZE)));

static uintptr_t kernel_page_table[KERNEL_INIT_PT_COUNT * PT_SIZE]
    __attribute__((aligned(PAGE_SIZE)));

extern uintptr_t _kernel_end;
extern uintptr_t _virt_kernel_addr;

static uintptr_t virt_kernel_addr;
static uintptr_t kernel_end_addr __attribute__((aligned(PAGE_SIZE)));
static uintptr_t pt_base_addr __attribute__((aligned(PAGE_SIZE)));

uintptr_t to_phys_addr(void *virt_addr) {
    return ((uintptr_t)virt_addr -
            (virt_kernel_addr ? virt_kernel_addr : VADDR));
}

void paging_init() {
    asm volatile ("mov $_kernel_end, %0" : "=r" (kernel_end_addr));
    asm volatile ("mov $_virt_kernel_addr, %0" : "=r" (virt_kernel_addr));
    pt_base_addr = kernel_end_addr + PAGE_SIZE;
    heap_init(pt_base_addr + (ARR_SIZE(page_dir)*PT_SIZE*PAGE_SIZE));
    frame_alloc_init();

    for (unsigned int i = 0; i < ARR_SIZE(page_dir); i++) {
        page_dir[i] |= R_W;
    }

    for (unsigned int i = 0; i < ARR_SIZE(kernel_page_table); i++) {
        kernel_page_table[i] = alloc_frame(i * 0x1000, USER | R_W | PRESENT);
    }

    for (unsigned int i = 0; i < KERNEL_INIT_PT_COUNT; i++) {
        page_dir[768 + i] =
            to_phys_addr(&kernel_page_table[PT_SIZE * i])
            | PRESENT
            | R_W
            | USER
            ;
    }

    add_isr_handler(14, &page_fault, 0);

    load_page_dir(to_phys_addr(&page_dir));
    enable_paging();

    klog("Paging enabled\n");
}

static inline int tab_present(uintptr_t descriptor) {
    return descriptor & PRESENT;
}

static inline uintptr_t get_ident_phys_page_addr(uint16_t pde, uint16_t pte) {
    return (pde * 0x400000) + (pte * 0x1000);
}

int copy_page_data(uintptr_t page_addr);

uintptr_t *clone_page_table(uintptr_t *pt) {
    uintptr_t *new_pt = kmalloc_align(PAGE_SIZE, PAGE_SIZE);

    if (!new_pt) {
        return NULL;
    }

    for (unsigned int i = 0; i < PAGE_SIZE; i++) {
        new_pt[i] = pt[i];

        if (pt[i] & (USER | PRESENT)) {
            /* copy_page_data(pt[i]); */
        }
    }

    return new_pt;
}

uintptr_t *clone_page_dir(uintptr_t *pd) {
    uintptr_t *new_pd = kmalloc_align(PAGE_SIZE, PAGE_SIZE);

    if (!new_pd) {
        return NULL;
    }

    for (unsigned int i = 0; i < PAGE_SIZE; i++) {
        new_pd[i] = page_dir[i];

        if (page_dir[i] & (USER | PRESENT)) {
            /* clone_page_table((uintptr_t *)(void *)page_dir[i]); */
        }
    }

    return new_pd;
}
int pt_present(uint16_t pde) {
    return tab_present(page_dir[pde]);
}

int page_present(uint16_t pde, uint16_t pte) {
    return tab_present(((uintptr_t *)page_dir[pde])[pte]);
}

int map_page(page_table_t pt, uint16_t pte, uintptr_t map_addr) {
    pt[pte] = map_addr | PRESENT | R_W;

    return pte;
}

int map_page_ident(page_table_t pt, uint16_t pde, uint16_t pte) {
    map_page(pt, pte, get_ident_phys_page_addr(pde, pte));

    return pte;
}

int map_pt_ident(uint16_t pde) {
    uintptr_t table_addr = pt_base_addr + (pde * 0x1000);

    for (unsigned int i = 0; i < 1024; i++) {
        map_page_ident((page_table_t)table_addr, pde, i);
    }

    klog("Alloc page table\n");
    page_dir[pde] = to_phys_addr((void *)table_addr) | PRESENT | R_W;

    return pde;
 }

int non_present_page_hanler(uint16_t pde, uint16_t pte) {
    if (!pt_present(pde)) {
        map_pt_ident(pde);
    } else if (!page_present(pde, pte)) {
        map_page_ident((page_table_t)page_dir[pde], pde, pte);
    }

    return 1;
}

void page_fault(struct isr_handler_args args) {
    uintptr_t fault_addr;
    uint16_t pde;
    uint16_t pte;

    klog("Page fault\n");
    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));

    pde = fault_addr >> 22;
    fb_print_hex(pde);
    pte = (fault_addr >> 12) & 0x3ff;

    if (args.error ^ PRESENT) {
        non_present_page_hanler(pde, pte);
    } else if (args.error ^ R_W) {

    }
        
    fb_print_num(args.error);
}

