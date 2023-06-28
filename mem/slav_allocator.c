#include "mem/allocator.h"
#include <stddef.h>
#include <stdint.h>

enum slab_state {
    SLAB_EMPTY,
    SLAB_PARTIAL,
    SLAB_FULL,
};

struct slab {
    size_t size;
    void *addr;
    enum slab_state state;
    struct slab *next;
};

struct slab_cache {
    struct slab_cache *next;
    struct slab *slabs_free;
    struct slab *slabs_full;
    struct slab *slabs_partial;
    size_t size;
};

static uintptr_t slab_heap_addr;

struct slab_cache *caches;

struct slab_cache *slab_create(size_t size) {
    struct slab_cache *cache = caches;

    for ( ;cache && cache->slabs_free; cache = cache->next);

    if (!cache) {
        cache = kmalloc(sizeof(*cache));
    }

    cache->slabs_free = kmalloc(sizeof(*(cache->slabs_free)));
    
    return cache;
}

int slab_alloc_slab(struct slab_cache *cache) {
}

void *slab_alloc_from_cache(struct slab_cache *cache) {
    if (!cache) {
        return NULL;
    }

    struct slab **non_full_slabs = &cache->slabs_partial;
    
    if (!*non_full_slabs) {
        *non_full_slabs = cache->slabs_free;
    }

    if (!*non_full_slabs) {
        slab_alloc_slab(cache);
        *non_full_slabs = cache->slabs_free;
    }

    void *addr = (*non_full_slabs)->addr;
    *non_full_slabs = (*non_full_slabs)->next;
    return addr;
}

void *slab_alloc(size_t size) {
    struct slab_cache *cache = caches;

    for(; cache && cache->size != size; cache = cache->next); 

    if (!cache) {
        return NULL;
    }
    
    return slab_alloc_from_cache(cache);
}

int slab_alloc_init(uintptr_t base) {
    caches = kmalloc(sizeof(*caches));

    if (!caches) {
        return 1;
    }
    
    slab_heap_addr = base;
}
