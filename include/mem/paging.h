#include <stdint.h>
#include "drivers/int.h"

void paging_init(void);

void page_fault(struct isr_handler_args args);

extern void *vga_buf;
