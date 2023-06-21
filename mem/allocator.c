#include <stdint.h>
#include <stdbool.h>
#include "drivers/fb.h"
#include "mem/paging.h"
#include "lib/slibc.h"

#define BLOCK_HEADER_MAGIC_NUM 0x0BADBABA
#define INIT_HEAP_SIZE (PAGE_SIZE * PT_SIZE)

static uintptr_t heap_base_addr;
static struct block_header *heap_base_block;
static uintptr_t heap_end_addr;

struct block_header {
    int magic_num;
    struct block_header *next;
    struct block_header *prev;
    struct block_header *next_hole;
    size_t size;
    uint8_t is_hole;
};

void write_header(struct block_header *head, struct block_header *next,
                        struct block_header *prev,
                        struct block_header *next_hole,
                        size_t size, uint8_t is_hole) {
    if (!head) return;
    
    head->next_hole = next_hole;
    head->is_hole = is_hole;
    head->next = next;
    head->prev = prev;
    head->size = size;
}

void heap_init(uintptr_t heap_base) {
    heap_base_addr = heap_base;
    heap_end_addr = heap_base_addr + INIT_HEAP_SIZE;

    heap_base_block = ((struct block_header *)(void *)heap_base_addr);
    
    heap_base_block[0] = (struct block_header) {
        .magic_num = BLOCK_HEADER_MAGIC_NUM,
        .next = NULL,
        .prev = NULL,
        .next_hole = NULL,
        .is_hole = true,
        .size = heap_end_addr - heap_base_addr,
    };
}

void *kmalloc(size_t siz) {
    if (siz == 0) return NULL;

    struct block_header *header;
    
    for (header = heap_base_block;
         ;
         header = header->next_hole) {

        if (header->is_hole && header->size >= siz) {
            uintptr_t data_base = ((uintptr_t)header + sizeof(*header));
            header->is_hole = false;
            header->size = siz + sizeof(*header);

            if (!header->next || header->size >= siz + 2*sizeof(*header)) {
                struct block_header *new_block =
                    (void *)(data_base + siz);

                header->next_hole = new_block;
                header->next = new_block;

                if (!heap_base_block->next_hole
                    || heap_base_block->next_hole == header) {
                    heap_base_block->next_hole = new_block;
                }

                *new_block = (struct block_header) {
                    .magic_num = BLOCK_HEADER_MAGIC_NUM,
                    .is_hole = true,
                    .size = heap_end_addr - data_base - siz,
                    .prev = header,
                    .next = header->next,
                    .next_hole = header->next_hole,
                };
            }

            return (void *)data_base;
        }
    }
/*
    heap_end_addr += PAGE_SIZE;

    if (siz > PAGE_SIZE) {
        heap_end_addr += (size_t)(siz / PAGE_SIZE) * PAGE_SIZE;
    }

    header = (struct block_header *)(void *)
        ((uintptr_t)header + header->size + sizeof(*header));
        
    uintptr_t data_base = ((uintptr_t)header + sizeof(*header));
        struct block_header *new_block =
        (void *)(data_base + siz);

    header->next_hole = new_block;
    header->next = new_block;
    header->is_hole = false;
    header->size = siz + sizeof(*header);

    if (!heap_base_block->next_hole
        || heap_base_block->next_hole == header) {
        heap_base_block->next_hole = new_block;
    }

    *new_block = (struct block_header) {
        .magic_num = BLOCK_HEADER_MAGIC_NUM,
        .is_hole = true,
        .size = heap_end_addr - data_base - siz,
        .prev = header,
        .next = header->next,
        .next_hole = header->next_hole,
    };
*/
    return NULL;
}

static inline void *get_block_header_addr(void *addr) {
    return (void *)((uintptr_t)addr - sizeof(struct block_header));
}

void kfree(void *addr) {
    if (!addr) return;

    if ((uintptr_t)addr > heap_end_addr || (uintptr_t)addr < heap_base_addr) {
        return;
    }
    
    void *block_addr = get_block_header_addr(addr);
    struct block_header *header = (struct block_header *)block_addr;
    
    if (header->magic_num != BLOCK_HEADER_MAGIC_NUM) return;

    header->is_hole = true;

    if (to_uintptr(heap_base_block->next_hole) > to_uintptr(header)) {
        header->next_hole = heap_base_block->next_hole;
        heap_base_block->next_hole = header;
    }

    if (header->next && header->next->is_hole) {
        header->next = header->next->next;
        header->next_hole = header->next_hole;
        header->size += header->next->size;
        header->next->magic_num = 0;

        if (header->next->next) {
            header->next->prev = header;
        }
    }

    if (header->prev && header->prev->is_hole) {
        header->prev->next = header->next;
        header->prev->size += header->size;
        header->prev->next_hole = header->next_hole;
        header->magic_num = 0;
        header->prev = header->prev->prev;

        if (header->next) {
            header->next->prev = header->prev;
        }
    }
}


