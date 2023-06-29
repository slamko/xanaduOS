#ifndef SLAB_ALLOC_H
#define SLAB_ALLOC_H

#include <stdint.h>
#include <stdlib.h>

struct slab;
struct slab_cache;

struct slab_cache *slab_cache_create(size_t size);

void slab_cache_destroy(struct slab_cache *cache);

void *slab_alloc_from_cache(struct slab_cache *cache);

void slab_free(struct slab_cache *cache, void *obj);

int slab_alloc_init(uintptr_t base);

#endif
