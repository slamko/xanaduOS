#ifndef BUDDY_ALLOC_H
#define BUDDY_ALLOC_H

#include <stdint.h>
#include <stddef.h>
#include "mem/paging.h"

void buddy_test(size_t mem);

static inline void set_addrs(uintptr_t *addrs, uintptr_t base,
                             size_t size, uint16_t flags) {
    for (unsigned int i = 0; i < size; i++) {
        addrs[i] = (base + (i * PAGE_SIZE)) | flags;
    }
}

int buddy_alloc_init(size_t mem_start, size_t mem_limit);

int buddy_alloc_frames(uintptr_t *addrs, size_t nframes, uint16_t flags);

int buddy_alloc_frame(uintptr_t *addr, uint16_t flags);

int buddy_alloc_at_addr(uintptr_t base, uintptr_t *addrs, size_t nframes,
                        uint16_t flags);

void buddy_free_frames(uintptr_t addr, size_t nframes);

void buddy_free_frame(uintptr_t addr);

#endif
