#ifndef MMAP_H
#define MMAP_H

#include "mem/paging.h"
#include "fs/fs.h"
#include <stddef.h>
#include <stdint.h>


int knmmap(struct page_dir *pd, uintptr_t *virt_addr, uintptr_t phys_addr,
           size_t page_num, uint16_t flags);
 
int kmmap(struct page_dir *pd, uintptr_t *virt_addr, uintptr_t phys_addr,
          uint16_t flags);
 

int kfsmmap(struct fs_node *node, uintptr_t *virt_addr, size_t *off, uint16_t flags);

void knmunmap(struct page_dir *pd, uintptr_t virt_addr, size_t page_num);

int kmmap_init();

extern struct buddy_alloc *user_buddy;

extern struct buddy_alloc *kern_buddy;

#endif
