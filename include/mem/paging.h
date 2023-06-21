#include <stdint.h>
#include <stdlib.h>
#include "drivers/int.h"

#define PT_SIZE 1024
#define PAGE_SIZE 4096

#define to_uintptr(ptr) ((uintptr_t)(void *)(ptr))

void paging_init(void);

void page_fault(struct isr_handler_args args);

void *kalloc(size_t siz);

void kfree(void *addr);

extern void *vga_buf;
