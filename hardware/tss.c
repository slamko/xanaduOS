#include "drivers/tss.h"
#include "drivers/fb.h"
#include "bin/shell.h"
#include <stdint.h>

extern void *kernel_stack_start;
extern void *kernel_stack_end;
extern void *kernel_int_stack_end;

void ltr(void);

struct tss_entry tss __attribute__((aligned(4096)));
void syscall(void);

void usermode(void) {
    /* fb_print_num(tss.cs); */
    /* asm volatile("cli"); */
    syscall();
    
    while(1);
}

void load_tss(void) {
    tss = (struct tss_entry){0}; 
    tss.ss0 = 0x10;
    tss.esp0 = (uint32_t)kernel_int_stack_end;

    tss.cs = 0x8 | 3;
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x10;

    ltr();
}

void set_kernel_stack(uint32_t stack)
{
   tss.esp0 = stack;
}

