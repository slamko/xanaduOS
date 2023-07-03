#ifndef SLAB_ALLOC_H
#define SLAB_ALLOC_H

#include <stdint.h>
#include <stddef.h>

struct slab {
    size_t size;
    size_t num_free;
    struct slab_chunk *chunks;
    struct slab *next;
};

struct slab_cache {
    struct slab_cache *next;
    struct slab_cache *prev;
    struct slab *slabs_free;
    struct slab *slabs_full;
    struct slab *slabs_partial;
    size_t size;
    size_t alignment;
};

struct slab_cache *slab_cache_create(size_t size);

void slab_cache_destroy(struct slab_cache *cache);

void *slab_alloc_from_cache(struct slab_cache *cache);

struct slab_cache *slab_cache_create_align(size_t size, size_t alignment);

void slab_free(struct slab_cache *cache, void *obj);

int slab_alloc_init(uintptr_t base);

#endif
