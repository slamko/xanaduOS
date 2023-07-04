#ifndef MMAP_H
#define MMAP_H

#include "mem/paging.h"
#include <stdint.h>


int knmmap(struct page_dir *pd, uintptr_t *virt_addr, uintptr_t phys_addr,
           size_t page_num, uint16_t flags);
 
int kmmap(struct page_dir *pd, uintptr_t *virt_addr, uintptr_t phys_addr,
          uint16_t flags);
 

void knmunmap(struct page_dir *pd, uintptr_t virt_addr, size_t page_num);

int kmmap_init(size_t mem_limit);

#endif