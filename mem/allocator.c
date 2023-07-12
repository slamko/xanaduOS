#include "mem/allocator.h"
#include "kernel/error.h"
#include "drivers/fb.h"
#include "lib/kernel.h"
#include "lib/slibc.h"
#include "mem/mmap.h"
#include "mem/paging.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BLOCK_HEADER_MAGIC_NUM 0x0BADBABA
#define MAP_CRITICAL_DIST 0x100000

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
                  struct block_header *prev, struct block_header *next_hole,
                  size_t size, uint8_t is_hole) {
    if (!head)
        return;

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

    klog("alloc bytes: %u\n", size);
    klog("at address: %x\n", t);

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
    heap_end_addr = map_limit;

    heap_base_block = to_header_ptr(heap_base_addr);

    memset(heap_base_block, 0, sizeof(*heap_base_block));

    heap_base_block[0].magic_num = BLOCK_HEADER_MAGIC_NUM;
    heap_base_block[0].is_hole = true;
    heap_base_block[0].size = heap_end_addr - heap_base_addr;
    heap_base_block[0].next_hole = NULL;
    heap_base_block[0].next = NULL;
}

static inline int expand_heap(struct block_header *header, size_t pt_num) {
    uintptr_t prev_end_addr = heap_end_addr;
    heap_end_addr += PT_SIZE * PAGE_SIZE;
    header->size += PT_SIZE * PAGE_SIZE;

    return mmap_pages(prev_end_addr, PT_SIZE * pt_num);
}

void *do_alloc(struct block_header *header, size_t size, uintptr_t data_base) {

    if (!header->next ||
        (header->size >= size + (3 * sizeof(*header)) && header->next)) {

        /* klog("Insert new header %x\n", header->next); */

        struct block_header *new_block = (void *)(data_base + size);
        if (!heap_base_block->next_hole ||
            heap_base_block->next_hole == header) {

            heap_base_block->next_hole = new_block;
        }

        memset(new_block, 0, sizeof(*new_block));
        new_block->magic_num = BLOCK_HEADER_MAGIC_NUM;
        new_block->is_hole = true;
        new_block->size = heap_end_addr - data_base - size;

        new_block->prev = header;
        new_block->next = header->next;
        if (new_block->next == (void *)0xFFFFFFFF) {
            new_block->next = NULL;
        }
        
        new_block->next_hole = header->next_hole;
        if (new_block->next_hole == (void *)0xFFFFFFFF) {
            new_block->next_hole = NULL;
        }

        header->next_hole = new_block;
        header->next = new_block;

        /* klog("Insert new header %x\n", header->next); */
    }

    header->is_hole = false;
    header->size = size + sizeof(*header);

    if (to_uintptr(header->next) > heap_end_addr - MAP_CRITICAL_DIST) {
        /* klog_warn("Running out of heap\n"); */

        klog("Running out of heap %x\n", header->next->size);
        klog("Mapped additional heap space %x\n", heap_end_addr);

        if (expand_heap(header, 1)) {
            panic("Could not expand kernel heap\n", 0);
        }
    }

    return (void *)data_base;
}

static inline uintptr_t get_data_base(struct block_header *header) {
    return to_uintptr(header) + sizeof(*header);
}

void *kmalloc_align(size_t siz, size_t alignment) {
    if (siz == 0) {
        return NULL;
    }

    if (alignment == 0) {
        alignment = 1;
    }

    struct block_header *header;
    struct block_header *last_header = NULL;
    size_t aligned_alloc_size = align_up(siz, alignment);

    for (header = heap_base_block; header;
         header = header->next_hole) {

        uintptr_t data_base = get_data_base(header);
        data_base = align_up(data_base, alignment);

        /* klog("Aligned alloc size %x\n", header->size); */

        if (header->is_hole &&
            header->size >=
                (aligned_alloc_size + (data_base - (uintptr_t)header))) {
            return do_alloc(header, aligned_alloc_size, data_base);
        }

        last_header = header;
    }

    klog("Not enough heap memory %x\n", last_header->size);
    size_t heap_expand_size =
        div_align_up(aligned_alloc_size, PT_SIZE * PAGE_SIZE) * 2;
    klog("Heap expand size %x\n", heap_expand_size);

    if (expand_heap(last_header, heap_expand_size)) {
        panic("Could not expand kernel heap\n", 0);
    }
    klog("Expanded heap %x\n", heap_end_addr);
 
    uintptr_t data_base = get_data_base(header);
    klog("Mapped additional heap space %x\n", heap_end_addr);

    return do_alloc(last_header, aligned_alloc_size, data_base);
}

void *kmalloc(size_t siz) { return kmalloc_align(siz, HEAP_ALIGN); }

void *kmalloc_align_phys(size_t siz, size_t align, uintptr_t *phys) {
    void *virt = kmalloc_align(siz, align);
    *phys = ptr_to_phys_addr(cur_pd, virt);

    return virt;
}

void *kmalloc_phys(size_t siz, uintptr_t *phys) {
    void *virt = kmalloc(siz);
    *phys = ptr_to_phys_addr(cur_pd, virt);

    return virt;
}

void *kzalloc_align(size_t val, size_t align, size_t size) {
    void *ret = kmalloc_align(size, align);

    if (!ret) {
        return ret;
    }

    memset(ret, val, size);
    return ret;
}

void *kzalloc(size_t val, size_t size) {
    void *ret = kmalloc(size);
    if (!ret) {
        return ret;
    }

    memset(ret, val, size);
    return ret;
}

static inline void *get_block_header_addr(void *addr) {
    return (void *)((uintptr_t)addr - sizeof(struct block_header));
}

void kfree(void *addr) {
    if (!addr)
        return;

    if ((uintptr_t)addr > heap_end_addr || (uintptr_t)addr < heap_base_addr) {
        return;
    }

    void *block_addr = get_block_header_addr(addr);
    struct block_header *header = (struct block_header *)block_addr;

    if (header->magic_num != BLOCK_HEADER_MAGIC_NUM)
        return;

    header->is_hole = true;

    if (to_uintptr(heap_base_block->next_hole) > to_uintptr(header)) {
        header->next_hole = heap_base_block->next_hole;
        heap_base_block->next_hole = header;
    } else {
        struct block_header *prev_hole = heap_base_block;
        for (; to_uintptr(prev_hole->next_hole) < to_uintptr(header);
             prev_hole = prev_hole->next_hole)
            ;

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

size_t check_block_size(void *addr) {
    if (!addr)
        return 0;

    if ((uintptr_t)addr > heap_end_addr || (uintptr_t)addr < heap_base_addr) {
        return 0;
    }

    void *block_addr = get_block_header_addr(addr);
    struct block_header *header = (struct block_header *)block_addr;

    if (header->magic_num != BLOCK_HEADER_MAGIC_NUM)
        return 0;

    if (header->is_hole)
        return 0;

    return header->size;
}

void ktest() {}
