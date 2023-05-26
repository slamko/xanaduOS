#include "drivers/tss.h"
#include <stdint.h>

extern void *kernel_stack_end;

struct tss_entry tss;

void usermode(void) {
    /* asm volatile("int $0x9"); */
    /* asm volatile("cli"); */

    /* while(1); */
}

void load_tss(void) {
    tss.ss0 = 0x10;
    tss.esp0 = (uintptr_t)kernel_stack_end;

    asm volatile ("movw $0x28, %ax");
    asm volatile ("ltr %ax");
}

