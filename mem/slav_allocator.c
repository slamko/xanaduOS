#include "mem/allocator.h"
#include "mem/slab_allocator.h"
#include "mem/paging.h"
#include <stddef.h>
#include <stdint.h>

struct slab_chunk {
    struct slab_chunk *next;
    struct slab_chunk *next_free;
    struct slab *slab;
    uintptr_t data_addr;
};

#define SLAB_CAPACITY 16

static uintptr_t slab_heap_addr;
struct slab_cache *caches;

#define to_chunk_ptr(uint_ptr) ((struct slab_chunk *)(void *)(uint_ptr))

int slab_alloc_slab(struct slab_cache *cache) {
    struct slab *next_slab = cache->slabs_free;
    size_t chunk_meta_size = (cache->size + sizeof(struct slab_chunk));
    
    size_t slab_data_size = SLAB_CAPACITY * chunk_meta_size;
    cache->slabs_free = kmalloc(sizeof(*cache->slabs_free) + slab_data_size);

    uintptr_t slab_chunks_addr =
        to_uintptr(cache->slabs_free) + sizeof(*cache->slabs_free);

    struct slab_chunk **cur_chunk = &cache->slabs_free->chunks;
    struct slab_chunk *prev_chunk = NULL;

    // insert the data chunks linked list inside the kmalloc
    // allocated space
    for (unsigned int i = 0; i < SLAB_CAPACITY; i++) {
        uintptr_t chunk_addr = slab_chunks_addr + (i * chunk_meta_size);

        *cur_chunk = to_chunk_ptr(chunk_addr);

        (*cur_chunk)->data_addr = chunk_addr + sizeof(struct slab_chunk);
        (*cur_chunk)->next = to_chunk_ptr(chunk_addr + chunk_meta_size);
        (*cur_chunk)->next_free = (*cur_chunk)->next;
        /* (*cur_chunk)->prev = prev_chunk; */
        (*cur_chunk)->slab = cache->slabs_free;

        /* prev_chunk = *cur_chunk; */
        cur_chunk = &(*cur_chunk)->next;
    }
        
    cache->slabs_free->next = next_slab;
    cache->slabs_free->num_free = SLAB_CAPACITY;

    return 0;
}

static inline int slab_remove_from_cache(struct slab **slab_list) {
    if (!*slab_list) {
        return 1;
    }
    
    *slab_list = (*slab_list)->next; 
    return 0;
}

static inline int slab_insert_in_cache(struct slab **slab_list,
    struct slab *slab) {

    if (!*slab_list) {
        return 1;
    }
    
    struct slab *next = (*slab_list)->next;
    *slab_list = slab; 
    (*slab_list)->next = next;
    return 0;
}

struct slab_cache *slab_cache_create(size_t size) {
    if (!caches) {
        caches = kmalloc(sizeof(*caches));
    }

    struct slab_cache *next_cache = caches->next;
    caches = kmalloc(sizeof(*caches));
    caches->next = next_cache;
    slab_alloc_slab(caches);
    caches->size = size;
    
    return caches;
}

void slab_cache_destroy(struct slab_cache *cache) {
}

void *slab_alloc_from_cache(struct slab_cache *cache) {
    if (!cache) {
        return NULL;
    }

    struct slab **non_full_slabs = &cache->slabs_partial->next;
    
    if (!*non_full_slabs) {
        *non_full_slabs = cache->slabs_free;
    }

    // no more free or partially free slabs
    if (!*non_full_slabs) {
        slab_alloc_slab(cache);
        *non_full_slabs = cache->slabs_free;
    }

    struct slab_chunk **free_chunk = &(*non_full_slabs)->chunks->next_free;
    uintptr_t free_addr = ((*free_chunk)->data_addr);
    *free_chunk = (*free_chunk)->next_free;
    (*non_full_slabs)->num_free--;

    // it was the last free chunk in the slab
    // adding the slab to full slabs linked list
    if (!*free_chunk) {
        slab_insert_in_cache(&cache->slabs_full, *non_full_slabs);
        slab_remove_from_cache(non_full_slabs);
    } else if ((*non_full_slabs)->num_free == SLAB_CAPACITY - 1) {
        slab_insert_in_cache(&cache->slabs_partial, *non_full_slabs);
        slab_remove_from_cache(non_full_slabs);
    }
    
    return (void *)free_addr;
}

void slab_free(struct slab_cache *cache, void *obj) {
    uintptr_t chunk_addr = (uintptr_t)obj - sizeof(struct slab_chunk);
    struct slab_chunk *chunk = to_chunk_ptr(chunk_addr);
    struct slab_chunk *first_chunk = chunk->slab->chunks;
    chunk->slab->num_free++;

    if (chunk->slab == cache->slabs_full) {
        slab_insert_in_cache(&cache->slabs_partial, chunk->slab);
        slab_remove_from_cache(&cache->slabs_full);
    } else if (chunk->slab->num_free == SLAB_CAPACITY) {
        slab_insert_in_cache(&cache->slabs_free, chunk->slab);
        slab_remove_from_cache(&cache->slabs_partial);
    }
    
    chunk->next_free = first_chunk->next_free;
    if (to_uintptr(first_chunk->next_free) > to_uintptr(chunk)) {
        first_chunk->next_free = chunk;
    }
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
    return 0;
}
