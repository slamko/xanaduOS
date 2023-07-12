#include "mem/mmap.h"
#include "drivers/initrd.h"
#include "drivers/int.h"
#include "kernel/error.h"
#include "mem/allocator.h"
#include "proc/proc.h"
#include "mem/paging.h"
#include "lib/kernel.h"
#include "mem/frame_allocator.h"
#include "mem/buddy_alloc.h"
#include "fs/fs.h"
#include <stddef.h>
#include <stdint.h>

struct buddy_alloc *kern_buddy;

static size_t knmmap_table(struct buddy_alloc *buddy, struct page_dir *pd,
                 uintptr_t **pt_ptr, uintptr_t *virt_addr,
                 size_t page_num, uint16_t flags) {
    int ret = 0;
    uint16_t pde, pte;
    page_table_t pt;

    if (page_num > PT_SIZE) {
        klog_error("Too much pages requested");
    }

    size_t pt_num = div_align_up(page_num, PT_SIZE);

    if ((ret = buddy_alloc_frames(buddy, virt_addr, page_num, 0))) {
        return ret;
    }
   
    klog("Alloc virt addr %x\n", *virt_addr);
    get_pde_pte(*virt_addr, &pde, &pte);

    ret = map_alloc_npt(pd, &pt, pt_num, pde, flags);
    if (ret) {
        return ret;
    }

    *pt_ptr = &pt[pte];

    return ret;
}

int knmmap(struct buddy_alloc *buddy, struct page_dir *pd,
           uintptr_t *virt_addr, uintptr_t phys_addr,
           size_t page_num, uint16_t flags) {

    int ret = 0;
    page_table_t pt;

    ret = knmmap_table(buddy, pd, &pt, virt_addr, page_num, flags);
    if (ret) {
        return ret;
    }

    if (phys_addr) {
        ret = alloc_nframes(page_num, phys_addr, pt, flags);
    } else {
        ret = find_alloc_nframes(page_num, pt, flags);
    }

    /* klog("Mmap: Alloc phys addr %x\n", *pt); */

    if (ret) {
        return ret;
    }

    flush_pages_contiguous(*virt_addr, page_num);
    return ret;
}

int kmmap(struct buddy_alloc *buddy, struct page_dir *pd,
          uintptr_t *virt_addr, uintptr_t phys_addr, uint16_t flags) {
    return knmmap(buddy, pd, virt_addr, phys_addr, 1, flags);
}

int kfsmmap(struct buddy_alloc *buddy, struct fs_node *node,
            uintptr_t *virt_addr, size_t *off, uint16_t flags) {
    if (!node) {
        return -1;
    }
    
    int ret;
    page_table_t pt = NULL;
    size_t npages = div_page_align_up(node->size);

    ret = knmmap_table(buddy, cur_pd, &pt, virt_addr, npages, flags);
    if (ret) {
        return ret;
    }
    
    /* klog("[T %x\n", pt); */
    *off = mmap_fs(node, pt, npages, flags);
    /* klog("Fsmap Alloc phys addr %x\n", *pt); */

    flush_pages_contiguous(*virt_addr, npages);
    /* klog("Map fs file %s with size %u\n", node->name, node->size); */
    return ret;
}

void knmunmap_contiguous(struct buddy_alloc *buddy, struct page_dir *pd,
                         uintptr_t virt_addr, size_t page_num) {
    buddy_free_frames(buddy, virt_addr, page_num);

    uint16_t pde, pte;
    get_pde_pte(virt_addr, &pde, &pte);

    unmap_pages(pd, pde, pte, page_num);
    dealloc_nframes(virt_addr, page_num);
    flush_pages_contiguous(virt_addr, page_num);
}

void kfsmunmap(struct buddy_alloc *buddy, struct page_dir *pd,
               struct fs_node *node, uintptr_t virt_addr) {

    size_t page_num = div_page_align_up(node->size);
    buddy_free_frames(buddy, virt_addr, page_num);

    uint16_t pde, pte;
    get_pde_pte(virt_addr, &pde, &pte);

    unmap_pages(pd, pde, pte, page_num);
    munmap_fs(node, virt_addr);
    
    flush_pages_contiguous(virt_addr, page_num);
}
               
int map_pages(pte_t pde, pte_t pte, size_t npages) {
    /* klog_warn("Page table not present\n"); */

    if (pte) {
        klog_error("Invalid mmap request: alloc address not aligned\n");
        return EINVAL;
    }

    size_t pt_num = div_align_up(npages, PT_SIZE);
    for (size_t i = 0; i < pt_num; i++) {
        int ret;
        page_table_t pt;

        if (pde + i >= PT_SIZE) {
            klog("No more phys memory\n");
            return ENOMEM;
        }

        ret = map_alloc_pt(cur_pd, &pt, pde + i, R_W | PRESENT);
        if (ret) {
            klog_error("Page Table allocation failed\n");
            return ret;
        }

        klog("Page table mapped %x\n", pde);
        ret = find_alloc_nframes(npages, &pt[pte], R_W | PRESENT);
        if (ret) {
            klog_error("Frame allocation failed %x\n", pt[pte + 514]);
            return ret;
        }

        to_phys_addr(cur_pd, 0xC1040000);
        flush_pages(&pt[pte], npages);
    }
    return 0;
}

int mmap_pages(uintptr_t addr, size_t npages) {
    pte_t pde, pte;
    get_pde_pte(addr, &pde, &pte);

    klog("Map pages %x\n", addr);
    return map_pages(pde, pte, npages);
}

int mmap_page(uintptr_t addr) {
    pte_t pde, pte;
    get_pde_pte(addr, &pde, &pte);

    return map_pages(pde, pte, 1);
}

void kmunmap(struct buddy_alloc *buddy, struct page_dir *pd,
             uintptr_t virt_addr) {
    knmunmap_contiguous(buddy, pd, virt_addr, 1);
}

int kmmap_init() {
    int ret = 0;
    
    kern_buddy = buddy_alloc_create(0xD0000000, 0xFFFFFFFF);

    if (ret) {
        return ret;
    }

    return ret;
}
