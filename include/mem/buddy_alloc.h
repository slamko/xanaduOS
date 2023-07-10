#ifndef BUDDY_ALLOC_H
#define BUDDY_ALLOC_H

#include <stdint.h>
#include <stddef.h>
#include "mem/paging.h"

struct buddy_alloc;

void buddy_test(size_t mem);

static inline void set_addrs(uintptr_t *addrs, uintptr_t base,
                             size_t size, uint16_t flags) {
    for (unsigned int i = 0; i < size; i++) {
        addrs[i] = (base + (i * PAGE_SIZE)) | flags;
    }
}

void buddy_free_frames(struct buddy_alloc *buddy, uintptr_t addr, size_t nframes);

int buddy_alloc_frames(struct buddy_alloc *buddy,
                        uintptr_t *addrs, size_t nframes, uint16_t flags);

void buddy_free_frame(struct buddy_alloc *buddy, uintptr_t addr);

int buddy_alloc_frame(struct buddy_alloc *buddy, uintptr_t *addr, uint16_t flags);

struct buddy_alloc *buddy_alloc_create(size_t mem_start, size_t mem_limit);

struct buddy_alloc *buddy_alloc_clone(struct buddy_alloc *copy);

void buddy_alloc_clean(struct buddy_alloc *buddy);

#endif
