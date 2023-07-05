#include "mem/mmap.h"
#include "kernel/error.h"
#include "mem/paging.h"
#include "lib/kernel.h"
#include "mem/frame_allocator.h"
#include "mem/buddy_alloc.h"
#include "fs/fs.h"
#include <stddef.h>
#include <stdint.h>

struct buddy_alloc *kern_buddy;

int knmmap(struct page_dir *pd, uintptr_t *virt_addr, uintptr_t phys_addr,
           size_t page_num, uint16_t flags) {
    int ret = 0;
    page_table_t pt;
    uint16_t pde, pte;

    if ((ret = buddy_alloc_frames(kern_buddy, virt_addr, page_num, 0))) {
        return ret;
    }
   
    get_pde_pte(*virt_addr, &pde, &pte);

    ret = map_alloc_pt(pd, &pt, pde);
    if (ret) {
        return ret;
    }

    if (phys_addr) {
        ret = alloc_nframes(page_num, phys_addr, &pt[pte], flags);
    } else {
        ret = find_alloc_nframes(page_num, &pt[pte], flags);
    }
    klog("Alloc phys addr %x\n", pd->page_tables[pde]);

    if (ret) {
        return ret;
    }

    flush_pages(*virt_addr, page_num);
    return ret;
}

int kmmap(struct page_dir *pd, uintptr_t *virt_addr, uintptr_t phys_addr,
          uint16_t flags) {
    return knmmap(pd, virt_addr, phys_addr, 1, flags);
}

int kfsmmap(struct fs_node *node, uintptr_t *virt_addr, size_t *off,
            uint16_t flags) {
    if (!node) {
        return -1;
    }
    
    int ret;
    uint16_t pde, pte;
    page_table_t pt;

    size_t npages = (node->size / PAGE_SIZE);
    if (node->size % PAGE_SIZE) {
        npages++;
    }

    if ((ret = buddy_alloc_frames(kern_buddy, virt_addr, npages, 0))) {
        return ret;
    }

    get_pde_pte(*virt_addr, &pde, &pte);
    klog("KFs virt addr %x\n", *virt_addr);
 
    ret = map_alloc_pt(cur_pd, &pt, pde);
    if (ret) {
        return ret;
    }

    *off = mmap_fs(node, &pt[pte], npages, flags);
    klog("Mapped virt addr %x %x\n", ((page_table_t)cur_pd->page_tables_virt[pde])[pte], cur_pd->page_tables[pde]);

    flush_pages(*virt_addr, npages);
    klog("Map fs file %s with size %u\n", node->name, node->size);
    return 0;
}

void knmunmap(struct page_dir *pd, uintptr_t virt_addr, size_t page_num) {
    buddy_free_frames(kern_buddy, virt_addr, page_num);

    uint16_t pde, pte;
    get_pde_pte(virt_addr, &pde, &pte);

    for (unsigned int i = 0; i < page_num; i ++) {
        unmap_page(pd, pde, pte + i);
    }
    
    flush_pages(virt_addr, page_num);
}

void kmunmap(struct page_dir *pd, uintptr_t virt_addr) {
    knmunmap(pd, virt_addr, 1);
}

int kmmap_init(size_t mem_limit) {
    int ret = 0;
    
    kern_buddy = buddy_alloc_create(0xD0000000, 0xFFFFFFFF);
    if (ret) {
        return ret;
    }

    return ret;
}
