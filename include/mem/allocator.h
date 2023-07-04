#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdint.h>
#include <stddef.h>

#define INIT_HEAP_SIZE (PAGE_SIZE * PT_SIZE * 10)

void *alloc_test(size_t size);

void heap_init(uintptr_t heap_base);

void *kmalloc_align(size_t siz, size_t alignment);

void *kmalloc(size_t siz);

void *kmalloc_phys(size_t siz, uintptr_t *phys);

void *kzalloc_align(size_t val, size_t align, size_t size);

void *kzalloc(size_t val, size_t size);

void fa_test(size_t siz);

void *kmalloc_align_phys(size_t siz, size_t align, uintptr_t *phys);

void kfree(void *addr);

#endif
