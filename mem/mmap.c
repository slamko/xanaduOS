#include "mem/mmap.h"
#include "drivers/initrd.h"
#include "kernel/error.h"
#include "proc/proc.h"
#include "mem/paging.h"
#include "lib/kernel.h"
#include "mem/frame_allocator.h"
#include "mem/buddy_alloc.h"
#include "fs/fs.h"
#include <stddef.h>
#include <stdint.h>

struct buddy_alloc *kern_buddy;

struct buddy_alloc *user_buddy;

int knmmap_table(struct page_dir *pd, uintptr_t **pt_ptr,
                 uintptr_t *virt_addr, size_t page_num, uint16_t flags) {
    int ret = 0;
    uint16_t pde, pte;
    page_table_t pt;
    struct buddy_alloc *buddy = kern_buddy;

    if (flags & USER) {
        buddy = user_buddy;
    }

    if ((ret = buddy_alloc_frames(buddy, virt_addr, page_num, 0))) {
        return ret;
    }
   
    get_pde_pte(*virt_addr, &pde, &pte);

    ret = map_alloc_pt(pd, &pt, pde, flags);
    klog("Alloc phys addr %x\n", (pd->page_tables_virt[pde])[pte]);
    if (ret) {
        return ret;
    }

    *pt_ptr = &pt[pte];

    return ret;
}

int knmmap(struct page_dir *pd, uintptr_t *virt_addr, uintptr_t phys_addr,
           size_t page_num, uint16_t flags) {
    int ret = 0;
    page_table_t pt;

    ret = knmmap_table(pd, &pt, virt_addr, page_num, flags);
    if (ret) {
        return ret;
    }

    if (phys_addr) {
        ret = alloc_nframes(page_num, phys_addr, pt, flags);
    } else {
        ret = find_alloc_nframes(page_num, pt, flags);
    }

    klog("Mmap: Alloc phys addr %x\n", *pt);

    if (ret) {
        return ret;
    }

    flush_pages(virt_addr, page_num);
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
    page_table_t pt;


    size_t npages = (node->size / PAGE_SIZE);
    if (node->size % PAGE_SIZE) {
        npages++;
    }
    npages *= 2;

    ret = knmmap_table(cur_pd, &pt, virt_addr, npages, flags);
    if (ret) {
        return ret;
    }
    
    *off = mmap_fs(node, pt, npages, flags);
    /* klog("Fsmap Alloc phys addr %x\n", *pt); */

    flush_pages(virt_addr, npages);
    /* klog("Map fs file %s with size %u\n", node->name, node->size); */
    return ret;
}

void knmunmap(struct page_dir *pd, uintptr_t virt_addr, size_t page_num) {
    buddy_free_frames(kern_buddy, virt_addr, page_num);

    uint16_t pde, pte;
    get_pde_pte(virt_addr, &pde, &pte);

    for (unsigned int i = 0; i < page_num; i ++) {
        unmap_page(pd, pde, pte + i);
    }
    
    flush_pages_contiguous(virt_addr, page_num);
}

void kmunmap(struct page_dir *pd, uintptr_t virt_addr) {
    knmunmap(pd, virt_addr, 1);
}

int kmmap_init() {
    int ret = 0;
    
    kern_buddy = buddy_alloc_create(0xD0000000, 0xFFFFFFFF);
    user_buddy = buddy_alloc_create(0x100000, 0x40000000);

    if (ret) {
        return ret;
    }

    return ret;
}
