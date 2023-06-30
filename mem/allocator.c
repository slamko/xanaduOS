#include "mem/allocator.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "lib/kernel.h"
#include "drivers/fb.h"
#include "mem/paging.h"
#include "lib/slibc.h"

#define BLOCK_HEADER_MAGIC_NUM 0x0BADBABA

static const size_t HEAP_ALIGN = 16;

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

    int padding1;
    int padding2;
};

#define to_header_ptr(int_addr) ((struct block_header *)((void *)(int_addr)))

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

void *alloc_test(size_t size) {
    int *t = kmalloc(size);

    if (!t) {
        klog("null\n");
    }
    
    *t = size * 3;
    klog("alloc bytes: ");
    fb_print_num(size);
    klog("at address: ");
    fb_print_hex((uintptr_t)t);
    klog("val: ");
    fb_print_hex(t[0]);

    return t;
}

void allocator_test(void) {
    int *p1 = alloc_test(8);
    int *p2 = alloc_test(16);
    int *p3 = alloc_test(8);
    kfree(p2);
    int *p4 = alloc_test(8);

    kfree(p1);
    kfree(p4);
    kfree(p3);
    int *r2 = alloc_test(16);
}

void heap_init(uintptr_t heap_base) {
    heap_base_addr = heap_base;
    heap_end_addr = heap_base_addr + INIT_HEAP_SIZE;

    heap_base_block = to_header_ptr(heap_base_addr);
    
    memset(heap_base_block, 0, sizeof(*heap_base_block));

    heap_base_block[0].magic_num = BLOCK_HEADER_MAGIC_NUM;
    heap_base_block[0].is_hole = true;
    heap_base_block[0].size = heap_end_addr - heap_base_addr;  
}

void *kmalloc_align(size_t siz, size_t alignment) {
    if (siz == 0) return NULL;

    if (alignment == 0) {
        alignment = 1;
    }

    struct block_header *header;
    size_t aligned_alloc_size = siz - (siz % alignment);

    if (siz % alignment) {
        aligned_alloc_size += alignment;
    }
    
    for (header = heap_base_block;
         ;
         header = header->next_hole) {
        uintptr_t data_base = ((uintptr_t)header + sizeof(*header));
        if (data_base % alignment) {
            data_base += (alignment - (data_base % alignment));
        }

        if (header->is_hole
            && header->size >=
                (aligned_alloc_size + (data_base - (uintptr_t)header))) {

            /* fb_print_hex(header->size); */
            if (!header->next ||
                (header->size >= aligned_alloc_size + (3 * sizeof(*header))
                 && header->next)) {

                /* fb_print_black("insert new\n"); */
                struct block_header *new_block =
                    (void *)(data_base + aligned_alloc_size);
                if (!heap_base_block->next_hole
                    || heap_base_block->next_hole == header) {

                    heap_base_block->next_hole = new_block;
                }


                memset(new_block, 0, sizeof(*new_block));
                new_block->magic_num = BLOCK_HEADER_MAGIC_NUM;
                new_block->is_hole = true;
                new_block->size = heap_end_addr - data_base - aligned_alloc_size;
                new_block->prev = header;
                new_block->next = header->next;
                new_block->next_hole = header->next_hole;

                header->next_hole = new_block;
                header->next = new_block;
            }

            header->is_hole = false;
            header->size = aligned_alloc_size + sizeof(*header);

            return (void *)data_base;
        }
    }
    klog("not enough");

    heap_end_addr += PAGE_SIZE;

    if (aligned_alloc_size > PAGE_SIZE) {
        heap_end_addr +=
            (aligned_alloc_size - (aligned_alloc_size % PAGE_SIZE)) * 2;
    }

    header =
        to_header_ptr((uintptr_t)header + header->size + sizeof(*header));
        
    uintptr_t data_base = ((uintptr_t)header + sizeof(*header));
    struct block_header *new_block =
        (void *)(data_base + aligned_alloc_size);

    header->next_hole = new_block;
    header->next = new_block;
    header->is_hole = false;
    header->size = aligned_alloc_size + sizeof(*header);

    if (!heap_base_block->next_hole
        || heap_base_block->next_hole == header) {
        heap_base_block->next_hole = new_block;
    }

    memset(new_block, 0, sizeof(*new_block));
    new_block->magic_num = BLOCK_HEADER_MAGIC_NUM;
    new_block->is_hole = true;
    new_block->size = heap_end_addr - data_base - aligned_alloc_size;
    new_block->prev = header;
    new_block->next = header->next;
    new_block->next_hole = header->next_hole;

    return (void *)data_base;
}

void *kmalloc(size_t siz) {
    return kmalloc_align(siz, HEAP_ALIGN);
}

void *kmalloc_align_phys(size_t siz, size_t align, uintptr_t *phys) {
    void *virt = kmalloc_align(siz, align);
    *phys = to_phys_addr(virt);

    return virt;
}

void *kmalloc_phys(size_t siz, uintptr_t *phys) {
    void *virt = kmalloc(siz);
    *phys = to_phys_addr(virt);

    return virt;
}

void *kzalloc_align(size_t val, size_t align, size_t size) {
    void *ret = kmalloc_align(size, align);
    memset(ret, val, size);
    return ret;
}

void *kzalloc(size_t val, size_t size) {
    void *ret = kmalloc(size);
    memset(ret, val, size);
    return ret;
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
    } else {
        struct block_header *prev_hole = heap_base_block;
        for (; to_uintptr(prev_hole->next_hole) < to_uintptr(header);
            prev_hole = prev_hole->next_hole);

        header->next_hole = prev_hole->next_hole;
        prev_hole->next_hole = header;
    }

    if (header->next && header->next->is_hole) {
        header->size += header->next->size;
        header->next_hole = header->next->next_hole;
        header->next->magic_num = 0;

        if (header->next->next) {
            header->next->prev = header;
        }
        header->next = header->next->next;
    }

    if (header->prev && header->prev->is_hole) {
        header->prev->next = header->next;
        header->prev->size += header->size;
        header->prev->next_hole = header->next_hole;
        header->magic_num = 0;
        /* header->prev = header->prev->prev; */

        if (header->next) {
            header->next->prev = header->prev;
        }
    }
}


