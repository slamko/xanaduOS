#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdint.h>
#include <stdlib.h>

void heap_init(uintptr_t heap_base);

void *kmalloc(size_t siz);

void kfree(void *addr);

#endif
