#ifndef PAGING_H
#define PAGING_H

#include <stddef.h>
#include <stdint.h>
#include "drivers/int.h"

#define PT_SIZE 1024
#define PAGE_SIZE 4096

#define to_uintptr(ptr) ((uintptr_t)(void *)(ptr))

#define align(num, alignment) ((num) - ((num) % (alignment)))
#define page_align(num) align(PAGE_SIZE) 

typedef uint16_t pte_t;

enum PAGING_STRUCT_FLAGS {
    PRESENT              = (1 << 0),
    R_W                  = (1 << 1),
    USER                 = (1 << 2),
    PWT                  = (1 << 3),
    PCD                  = (1 << 4),
    ACCESSED             = (1 << 5),
    DIRTY                = (1 << 6),
    PS                   = (1 << 7),
    GLOBAL               = (1 << 8),
};

typedef uintptr_t * page_table_t;

struct page_dir {
    uintptr_t *page_tables;

    uintptr_t *page_tables_virt;

    uintptr_t pd_phys_addr;
};

uintptr_t to_phys_addr(void *virt_addr);

void paging_init(size_t pmem_amount);

void page_fault(struct isr_handler_args args);

int clone_page_dir(struct page_dir *pd, struct page_dir *new_pd);

static inline uintptr_t get_ident_phys_page_addr(uint16_t pde, uint16_t pte) {
    return (pde * 0x400000) + (pte * PAGE_SIZE);
}

int clone_page_table(page_table_t pt, page_table_t *new_pt_ptr,
                     uintptr_t *pt_phys_addr);

int clone_cur_page_dir(struct page_dir *new_pd);

int switch_page_dir(struct page_dir *pd);

int get_pde_pte(uintptr_t addr, uint16_t *pde_p, uint16_t *pte_p);

uintptr_t get_tab_pure_addr(uintptr_t table);

extern struct page_dir *cur_pd;

#endif
