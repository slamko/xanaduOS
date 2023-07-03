#ifndef MMAP_H
#define MMAP_H

#include "mem/paging.h"
#include <stdint.h>

int kmmap(struct page_dir *pd, uintptr_t *virt_addr, uintptr_t phys_addr);

#endif
