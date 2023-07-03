#include "mem/mmap.h"
#include "mem/paging.h"
#include "mem/frame_allocator.h"
#include <stddef.h>
#include <stdint.h>

struct mmap_bitmap {
    struct mmap_bitmap *next;
    uint32_t map;
};

int kmmap(struct page_dir *pd, uintptr_t *virt_addr, uintptr_t phys_addr) {
    int ret = 0;
    page_table_t pt;
    uint16_t pde, pte;
    
    *virt_addr = phys_addr; // fixme
    get_pde_pte(phys_addr, &pde, &pte);

    ret = map_alloc_pt(pd, &pt, pde);
    if (ret) {
        return ret;
    }

    ret = alloc_frame(phys_addr, &pt[pte], R_W | PRESENT);
    if (ret) {
        return ret;
    }

    flush_page(phys_addr);
    return ret;
}

int kmmap_init(size_t mem_limit) {
    
}
