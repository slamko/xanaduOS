#include "kernel/error.h"
#include "mem/slab_allocator.h"
#include "lib/kernel.h"
#include "mem/allocator.h"
#include "mem/paging.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>

struct slab_chunk {
    struct slab_chunk *next;
    struct slab_chunk *next_free;
    struct slab_chunk *prev_free;
    struct slab *slab;
    uintptr_t data_addr;
};

struct slab {
    size_t size;
    size_t num_free;
    struct slab *next;
    struct slab *prev;
    struct slab_chunk base_chunk;
    struct slab_chunk chunks[];
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

typedef struct slab_chunk slab_chunk_t;

#define SLAB_CAPACITY 16

static uintptr_t slab_heap_addr;
struct slab_cache *caches;

#define to_chunk_ptr(uint_ptr) ((struct slab_chunk *)(void *)(uint_ptr))

static inline int slab_remove_from_cache(struct slab **slab_list) {
    if (!*slab_list) {
        return 1;
    }
    
    *slab_list = (*slab_list)->next; 
    return 0;
}

static inline int slab_insert_in_cache(struct slab **slab_list,
    struct slab *slab) {

    struct slab *next = *slab_list;
    *slab_list = slab; 
    (*slab_list)->next = next;
    return 0;
}


int slab_alloc_slab_align(struct slab_cache *cache, size_t align) {
    struct slab *new_slab;
    size_t chunk_meta_size;
    size_t slab_data_size;
    void *slab_data;
    
    if (align) {
        size_t slab_chunks_size = (SLAB_CAPACITY + 1) * sizeof(slab_chunk_t);
        chunk_meta_size = cache->size;

        slab_data_size = SLAB_CAPACITY * cache->size;
        new_slab = kmalloc(sizeof(*new_slab) + slab_chunks_size);
        slab_data = kmalloc_align(slab_data_size, align);
    } else {
        chunk_meta_size = (cache->size + sizeof(struct slab_chunk));

        slab_data_size = ((SLAB_CAPACITY + 1) * chunk_meta_size) - cache->size;
        new_slab = kmalloc(sizeof(*new_slab) + slab_data_size);
    }

    uintptr_t slab_chunks_addr = to_uintptr(new_slab) + sizeof(*new_slab);
    struct slab_chunk **cur_chunk = (struct slab_chunk **)&new_slab->chunks;
    struct slab_chunk *prev_chunk = &new_slab->base_chunk;
    /* klog("Slab alloc\n"); */

    // insert the data chunks linked list inside the kmalloc
    // allocated space
    for (unsigned int i = 0; i < SLAB_CAPACITY; i++) {
        uintptr_t chunk_addr;
        struct slab_chunk *ch;
        
        if (align) {
            chunk_addr = slab_chunks_addr + (i * sizeof(*ch));
            *cur_chunk = to_chunk_ptr(chunk_addr);

            ch = *cur_chunk;

            ch->data_addr =
                (uintptr_t)slab_data + (i * chunk_meta_size);
            ch->next =
                to_chunk_ptr(chunk_addr + sizeof(slab_chunk_t));
        } else {
            chunk_addr = slab_chunks_addr + (i * chunk_meta_size);

            if (!*cur_chunk) {
                *cur_chunk = to_chunk_ptr(chunk_addr);
            }

            ch = *cur_chunk;
            ch->data_addr = chunk_addr;
            ch->next =
                to_chunk_ptr(chunk_addr + chunk_meta_size);
        }
        
        /* klog("Init data addrs: %p\n", ch->data_addr); */
        ch->next_free = ch->next;
        /* if (i > 2) */
        ch->prev_free = prev_chunk;
        ch->slab = new_slab;

        prev_chunk = *cur_chunk;
        cur_chunk = &ch->next;
    }
        
    /* klog("Chunk next free: %p\n", new_slab->chunks->next); */
    /* klog("Chunk next free: %p\n", new_slab->chunks->next_free); */
    new_slab->size = chunk_meta_size;
    new_slab->num_free = SLAB_CAPACITY;

    new_slab->base_chunk.data_addr = 0;
    new_slab->base_chunk.slab = new_slab;
    new_slab->base_chunk.next = new_slab->chunks;
    new_slab->base_chunk.next_free = new_slab->chunks;
    new_slab->base_chunk.prev_free = NULL;
    new_slab->next = NULL;
    new_slab->prev = NULL;

    slab_insert_in_cache(&cache->slabs_free, new_slab);

    return 0;
}

struct slab_cache *slab_cache_create_align(size_t size, size_t alignment) {
    struct slab_cache *next_cache = caches;
    caches = kzalloc(0, sizeof(*caches));
    /* klog("new: %x\n", caches); */
    caches->next = next_cache;
    if (caches->next) {
        caches->next->prev = caches;
    }
    
    if (alignment) {
        caches->size = align_up(size, alignment);
    } else {
        caches->size = size;
    }

    caches->alignment = alignment;
    slab_alloc_slab_align(caches, alignment);
    
    return caches;
}

struct slab_cache *slab_cache_create(size_t size) {
    return slab_cache_create_align(size, 0);
}

void slab_destroy_slab(struct slab_cache *cache, struct slab *slabs) {
    for (; slabs; slabs = slabs->next) {
        kfree(slabs);

        if (cache->alignment) {
            kfree((void *)slabs->chunks->data_addr);
        } 
    }
}

void slab_cache_destroy(struct slab_cache *cache) {
    if (cache->prev) {
        cache->prev->next = cache->next;
    }

    if (cache->next) {
        cache->next->prev = cache->prev;
    }

    slab_destroy_slab(cache, cache->slabs_free);
    slab_destroy_slab(cache, cache->slabs_partial);
    slab_destroy_slab(cache, cache->slabs_full);

    kfree(cache);
}

static void print_all_slabs(struct slab_cache *cache) {

    if (cache->slabs_partial) {
        klog("Partial slabs %x\n", cache->slabs_partial);
        klog("Partial slabs next %x\n", cache->slabs_partial->next);
    }
    if (cache->slabs_free) {
        klog("Free slabs next %x\n", cache->slabs_free->next);
        klog("Free slabs %x\n", cache->slabs_free);
    }
    if (cache->slabs_full) {
        klog("Full slabs next %x\n", cache->slabs_full->next);
        klog("Full slabs %x\n", cache->slabs_full);
    }
}

void *slab_alloc_from_cache(struct slab_cache *cache) {
    if (!cache) {
        return NULL;
    }

    struct slab *non_full_slabs = cache->slabs_partial;
    
    if (!non_full_slabs) {
        /* debug_log("No partially full slabs\n"); */
        non_full_slabs = cache->slabs_free;
    }

    // no more free or partially free slabs
    if (!non_full_slabs) {
        if (slab_alloc_slab_align(cache, cache->alignment)) {
            return NULL;
        }

        non_full_slabs = cache->slabs_free;
        debug_log("Alloc new slab %x\n", non_full_slabs);
    }

    struct slab_chunk **free_chunk = &non_full_slabs->base_chunk.next_free;
    /* klog("Slab alloc%x\n", (*free_chunk)->data_addr); */
    uintptr_t free_addr = ((*free_chunk)->data_addr);

    if ((*free_chunk)->next_free) {
        (*free_chunk)->next_free->prev_free = (*free_chunk)->prev_free;
    }
    *free_chunk = (*free_chunk)->next_free;
    non_full_slabs->num_free--;

    // it was the last free chunk in the slab
    // adding the slab to full slabs linked list
    if (non_full_slabs->num_free == 0) {
        klog("Last free chunk %x\n", non_full_slabs);
        slab_remove_from_cache(&cache->slabs_partial);
        slab_insert_in_cache(&cache->slabs_full, non_full_slabs);
        print_all_slabs(cache);
    } else if (non_full_slabs->num_free == SLAB_CAPACITY - 1) {
        slab_remove_from_cache(&cache->slabs_free);
        slab_insert_in_cache(&cache->slabs_partial, non_full_slabs);
        print_all_slabs(cache);
    }
    
    return (void *)free_addr;
}

bool addr_within_slab(struct slab *slab, uintptr_t addr) {
    uintptr_t slab_data_addr = slab->base_chunk.next->data_addr;
    return (slab_data_addr <= addr
            && (slab_data_addr + (slab->size * SLAB_CAPACITY)) > addr);
}

uintptr_t slab_find_chunk(struct slab *slab, uintptr_t seek_addr) {
    for (; slab; slab = slab->next) {
        if (addr_within_slab(slab, seek_addr)) {
            struct slab_chunk *chunk = slab->chunks->next;

            for(; chunk; chunk = chunk->next) {
                if (chunk->data_addr == seek_addr) {
                    return to_uintptr(chunk);
                }
            }
        }
    }

    return 0;
}

void slab_free(struct slab_cache *cache, void *obj) {
    uintptr_t chunk_addr = 0;
    uintptr_t obj_addr = (uintptr_t)obj;
    klog_warn("Slab free was called\n");

    if (!cache->alignment) {
        chunk_addr = obj_addr - sizeof(struct slab_chunk);
    } else {
        chunk_addr = slab_find_chunk(cache->slabs_partial, obj_addr);

        if (!chunk_addr) {
            chunk_addr = slab_find_chunk(cache->slabs_full, obj_addr);
        }

        /* klog("Chunk aligned addr: %p\n", chunk_addr); */
        if (!chunk_addr) {
            return;
        }
    }
    
    struct slab_chunk *chunk = to_chunk_ptr(chunk_addr);
    struct slab_chunk *first_chunk = &chunk->slab->base_chunk;
    chunk->slab->num_free++;

    if (chunk->slab->num_free == 1) {
        debug_log("FREE: move to partial list\n");
        slab_remove_from_cache(&cache->slabs_full);
        slab_insert_in_cache(&cache->slabs_partial, chunk->slab);
    } else if (chunk->slab->num_free == SLAB_CAPACITY) {
        debug_log("FREE: add free slab list\n");
        slab_remove_from_cache(&cache->slabs_partial);
        slab_insert_in_cache(&cache->slabs_free, chunk->slab);
    }
    
    if (!first_chunk->next_free ||
        to_uintptr(first_chunk->next_free) > to_uintptr(chunk)) {
        chunk->next_free = first_chunk->next_free;
        first_chunk->next_free = chunk;
    } else {
        struct slab_chunk *prev_free = first_chunk->next_free;
        for (; to_uintptr(prev_free->next_free) < to_uintptr(chunk);
             prev_free = prev_free->next_free);

        chunk->next_free = prev_free->next_free;
        prev_free->next_free = chunk;
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
    return 0;
}

void slab_test(void) {
/*
    struct slab_cache *cache = slab_cache_create_align(0x100, 0x1000);

    void *ps[20];
    for (unsigned int i = 0; i < 20; i++) {
        ps[i] = slab_alloc_from_cache(cache);
        klog("Slab alloc %u: %p\n", i, ps[i]);

        if (!(i % 3)) {
            slab_free(cache, ps[i]);
        }
    }
    slab_free(cache, ps[15]);
    ps[15] = slab_alloc_from_cache(cache);
    klog("Slab alloc: %p\n", ps[15]);

    slab_cache_destroy(cache);
*/
    struct slab_cache *cache = slab_cache_create_align(0x1000, 0x1000);
    void *ps[20];
   /* return; */
    /* ps[0] = slab_alloc_from_cache(cache); */
    /* klog("Slab alloc %u: %p\n", 0, ps[0]); */


    for (unsigned int i = 0; i < 20; i++) {
        ps[i] = slab_alloc_from_cache(cache);
        klog("Slab alloc %u: %p\n", i, ps[i]);

        if (!(i % 3)) {
            /* slab_free(cache, ps[i]); */
        }
    }

    slab_free(cache, ps[15]);
    ps[15] = slab_alloc_from_cache(cache);
    /* klog("Slab alloc: %p\n", ps[15]); */

    slab_cache_destroy(cache);
    /* klog("Hello"); */
    /* void *p2 = slab_alloc_from_cache(cache); */
    /* void *p3 = slab_alloc_from_cache(cache); */

    /*
    struct slab_cache *cache_align = slab_cache_create_align(0x1000, 0x1000);
    void *a1 = slab_alloc_from_cache(cache_align);
    void *a2 = slab_alloc_from_cache(cache_align);
    slab_free(cache_align, a1);
    void *a3 = slab_alloc_from_cache(cache_align);
    klog("Slab alloc aligned: %p, %p, %p\n", a1, a2, a3);
    */
}
