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

void page_fault(struct isr_handler_args args);

void load_page_dir(uintptr_t dir);
void enable_paging(void);
void disable_paging(void);

void print_cr0(void);

#define KERNEL_INIT_PT_COUNT 4
#define DEFAULT_FLAGS (R_W | PRESENT)

struct page_dir init_pd;
struct page_dir *cur_pd;

static uintptr_t init_page_tables[PT_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uintptr_t init_page_tables_virt[PT_SIZE]; 

static uintptr_t kernel_page_table[KERNEL_INIT_PT_COUNT * PT_SIZE]
    __attribute__((aligned(PAGE_SIZE)));

static uintptr_t virt_kernel_addr;
static uintptr_t rodata_start;
static uintptr_t rodata_end;
static uintptr_t kernel_end_addr __attribute__((aligned(PAGE_SIZE)));

static uintptr_t pt_base_addr __attribute__((aligned(PAGE_SIZE)));

uintptr_t to_phys_addr(void *virt_addr) {
    return ((uintptr_t)virt_addr -
            (virt_kernel_addr ? virt_kernel_addr : VADDR));
}

void page_tables_init(void) {
    for (unsigned int i = 0; i < ARR_SIZE(init_page_tables); i++) {
        init_pd.page_tables[i] |= R_W;
    }

    for (unsigned int i = 0; i < ARR_SIZE(kernel_page_table); i++) {
        uint16_t flags = PRESENT;
        uintptr_t paddr = i * 0x1000;

        if (paddr >= rodata_end || paddr < rodata_start) {
            flags |= R_W;
        }
        
        kernel_page_table[i] = alloc_frame(i * 0x1000, flags);
    }

    for (unsigned int i = 0; i < KERNEL_INIT_PT_COUNT; i++) {
        init_pd.page_tables[768 + i] =
            to_phys_addr(&kernel_page_table[PT_SIZE * i])
            | PRESENT
            | R_W
            ;

        init_pd.page_tables_virt[768 + i] =
            (uintptr_t)&kernel_page_table[PT_SIZE * i];
    }

    add_isr_handler(14, &page_fault, 0);

    load_page_dir(init_pd.pd_phys_addr);
    enable_paging();
}

void paging_init() {
    asm volatile ("mov $_kernel_end, %0" : "=r" (kernel_end_addr));
    asm volatile ("mov $_virt_kernel_addr, %0" : "=r" (virt_kernel_addr));
    asm volatile ("mov $_rodata_start, %0" : "=r" (rodata_start));
    asm volatile ("mov $_rodata_end, %0" : "=r" (rodata_end));
    rodata_start = to_phys_addr((void *)rodata_start);
    rodata_end = to_phys_addr((void *)rodata_end);

    pt_base_addr = kernel_end_addr + PAGE_SIZE;

    heap_init(pt_base_addr);
    frame_alloc_init();

    init_pd.page_tables_virt = init_page_tables_virt;
    init_pd.page_tables = init_page_tables;
    init_pd.pd_phys_addr = to_phys_addr(&init_page_tables);
    cur_pd = &init_pd;

    page_tables_init();
    
    klog("Paging enabled\n");
}

static inline int tab_present(uintptr_t descriptor) {
    return descriptor & PRESENT;
}

static inline uint16_t get_tab_flags(uintptr_t table) {
    return (table & 0xfff);
}

static inline uintptr_t get_tab_pure_addr(uintptr_t table) {
    return (table & ~0xfff);
}

int copy_page_data(uintptr_t src, uintptr_t dest);

int clone_page_table(page_table_t pt, page_table_t *new_pt_ptr,
                     uintptr_t *new_pt_phys_addr) {
    page_table_t new_pt;
    
    if (!new_pt_phys_addr || !new_pt_ptr) {
        return 1;
    }
    
    *new_pt_ptr = kmalloc_align_phys(PAGE_SIZE, PAGE_SIZE, new_pt_phys_addr);
    new_pt = *new_pt_ptr;

    if (!new_pt) {
        return 1;
    }

    for (unsigned int i = 0; i < PT_SIZE; i++) {
        uint16_t flags = get_tab_flags(new_pt[i]);

        if (pt[i] & USER && pt[i] & PRESENT) {
            new_pt[i] = find_alloc_frame(flags);
            
            if (!new_pt[i]) {
                /* return 1; */
            }
            
            uintptr_t paddr_pt = get_tab_pure_addr(pt[i]);
            uintptr_t paddr_new_pt = get_tab_pure_addr(new_pt[i]);
            copy_page_data(paddr_pt, paddr_new_pt);
        } else {
            new_pt[i] = pt[i];
        }
    }

    return 0;
}

int clone_page_dir(struct page_dir *pd, struct page_dir *new_pd) {
    uintptr_t ptables_phys;

    if (!new_pd) {
        return 1;
    }

    new_pd->page_tables = kmalloc_align_phys(PAGE_SIZE, PAGE_SIZE, &ptables_phys);
    new_pd->page_tables_virt = kmalloc(PAGE_SIZE);
    
    if (!new_pd->page_tables || !new_pd->page_tables_virt) {
        return 1;
    }

    new_pd->pd_phys_addr = ptables_phys;

    for (unsigned int i = 0; i < PT_SIZE; i++) {
        if (pd->page_tables[i] & USER && pd->page_tables[i] & PRESENT) {
            klog("user");
            page_table_t new_pt;
            uintptr_t new_pt_paddr;
            page_table_t pt = (uintptr_t *)(void *)pd->page_tables_virt[i];

            if (clone_page_table(pt, &new_pt, &new_pt_paddr)) {
                return 1;
            }
            new_pd->page_tables[i] = new_pt_paddr;
            new_pd->page_tables_virt[i] = to_uintptr(new_pt);
        } else {
            new_pd->page_tables_virt[i] = pd->page_tables_virt[i];
            new_pd->page_tables[i] = pd->page_tables[i];
            /* fb_print_hex(new_pd->page_tables[i]); */
        }
    }

    return 0;
}

int clone_cur_page_dir(struct page_dir *new_pd) {
    return clone_page_dir(cur_pd, new_pd);
}

int switch_page_dir_asm(uintptr_t pd);

int switch_page_dir(struct page_dir *pd) {
    fb_print_hex(pd->pd_phys_addr);
    cur_pd = pd;
    switch_page_dir_asm(pd->pd_phys_addr);
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

int get_pde_pte(uintptr_t addr, uint16_t *pde_p, uint16_t *pte_p) {
    if (!pde_p || !pte_p) {
        return 1;
    }
    
    *pde_p = addr >> 22;
    *pte_p = (addr >> 12) & 0x3ff;
    return 0;
}

int non_present_page_hanler(uint16_t pde, uint16_t pte) {
    if (!pt_present(cur_pd, pde)) {
        page_table_t pt;
        map_alloc_pt(cur_pd, &pt, pde);
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
        
    /* fb_print_num(args.error); */
}

