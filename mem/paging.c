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
#define DEFAULT_FLAGS (R_W | PRESENT)

struct page_dir init_pd;
struct page_dir *cur_pd;

static uintptr_t init_page_tables[PT_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uintptr_t init_page_tables_virt[PT_SIZE]; 

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

    heap_init(pt_base_addr);
    frame_alloc_init();

    init_pd.page_tables_virt = init_page_tables_virt;
    init_pd.page_tables = init_page_tables;
    init_pd.pd_phys_addr = to_phys_addr(&init_page_tables);
    cur_pd = &init_pd;

    for (unsigned int i = 0; i < ARR_SIZE(init_page_tables); i++) {
        init_pd.page_tables[i] |= R_W;
    }

    for (unsigned int i = 0; i < ARR_SIZE(kernel_page_table); i++) {
        kernel_page_table[i] = alloc_frame(i * 0x1000, USER | R_W | PRESENT);
    }

    for (unsigned int i = 0; i < KERNEL_INIT_PT_COUNT; i++) {
        init_pd.page_tables[768 + i] =
            to_phys_addr(&kernel_page_table[PT_SIZE * i])
            | PRESENT
            | R_W
            | USER
            ;

        init_pd.page_tables_virt[768 + i] =
            (uintptr_t)&kernel_page_table[PT_SIZE * i];
    }

    add_isr_handler(14, &page_fault, 0);

    load_page_dir(init_pd.pd_phys_addr);
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

int clone_page_dir(struct page_dir *pd, struct page_dir *new_pd) {
    uintptr_t ptables_phys;
    uintptr_t *new_pd_tables =
        kmalloc_align_phys(PAGE_SIZE, PAGE_SIZE, &ptables_phys);
    
    if (!new_pd_tables) {
        return 1;
    }

    new_pd->page_tables_virt = new_pd_tables;
    new_pd->pd_phys_addr = ptables_phys;

    for (unsigned int i = 0; i < PAGE_SIZE; i++) {
        new_pd->page_tables_virt[i] = pd->page_tables_virt[i];
        new_pd->page_tables[i] = pd->page_tables[i];

        if (pd->page_tables[i] & (USER | PRESENT)) {
            /* clone_page_table((uintptr_t *)(void *)page_dir[i]); */
        }
    }

    return 0;
}

uintptr_t *get_pd_page(struct page_dir *pd, uint16_t pde, uint16_t pte) {
    return &(((uintptr_t *)pd->page_tables_virt[pde])[pte]);
}

int pt_present(struct page_dir *pd, uint16_t pde) {
    return tab_present(pd->page_tables[pde]);
}

int page_present(struct page_dir *pd, uint16_t pde, uint16_t pte) {
    return tab_present(*get_pd_page(pd, pde, pte));
}

int non_present_page_hanler(uint16_t pde, uint16_t pte) {
    if (!pt_present(cur_pd, pde)) {
        page_table_t pt;
        cur_pd->page_tables[pde] = alloc_pt(&pt, R_W | PRESENT);
        cur_pd->page_tables_virt[pde] = to_uintptr(pt);
        map_frame(pt, pte, R_W | PRESENT);
    } else if (!page_present(cur_pd, pde, pte)) {
        uintptr_t *pt_entry = get_pd_page(cur_pd, pde, pte);
        *pt_entry = find_alloc_frame(R_W | PRESENT);
    }

    return 1;
}

void page_fault(struct isr_handler_args args) {
    uintptr_t fault_addr;
    uint16_t pde;
    uint16_t pte;

    /* klog("Page fault\n"); */
    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));

    pde = fault_addr >> 22;
    /* fb_print_hex(pde); */
    pte = (fault_addr >> 12) & 0x3ff;
    /* fb_print_hex(pte); */

    if (args.error ^ PRESENT) {
        non_present_page_hanler(pde, pte);
    } else if (args.error ^ R_W) {

    }
        
    fb_print_num(args.error);
}

