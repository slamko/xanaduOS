#ifndef MMAP_H
#define MMAP_H

#include "mem/buddy_alloc.h"
#include "mem/paging.h"
#include "fs/fs.h"
#include <stddef.h>
#include <stdint.h>


int knmmap(struct buddy_alloc *buddy, struct page_dir *pd,
           uintptr_t *virt_addr, uintptr_t phys_addr, size_t page_num, uint16_t flags);
 
int kmmap(struct buddy_alloc *buddy, struct page_dir *pd,
          uintptr_t *virt_addr, uintptr_t phys_addr, uint16_t flags);
 

int kfsmmap(struct buddy_alloc *buddy, struct fs_node *node,
            uintptr_t *virt_addr, size_t *off, uint16_t flags);

void knmunmap(struct buddy_alloc *buddy, struct page_dir *pd,
              uintptr_t *virt_addr, size_t page_num);

void knmunmap_contiguous(struct buddy_alloc *buddy, struct page_dir *pd,
                         uintptr_t virt_addr, size_t page_num);

int kmmap_init();

extern struct buddy_alloc *kern_buddy;

#endif
