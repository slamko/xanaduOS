#ifndef FRAME_ALLOC_H
#define FRAME_ALLOC_H

#include <stdint.h>
#include "mem/paging.h"

void frame_alloc_init(void);

uintptr_t alloc_frame(uintptr_t addr, unsigned int flags);

uintptr_t find_alloc_frame(unsigned int flags);

void map_frame(page_table_t pt, unsigned int pte, uint16_t flags);

uintptr_t alloc_pt(page_table_t *new_pt, uint16_t flags);

int map_alloc_pt(struct page_dir *pd, page_table_t *pt, uint16_t pde);

void map_frame(page_table_t pt, unsigned int pte, uint16_t flags);

int dealloc_frame(uintptr_t addr);

#endif
