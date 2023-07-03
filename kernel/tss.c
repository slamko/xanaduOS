#include "drivers/tss.h"
#include "kernel/syscall.h"
#include "lib/kernel.h"
#include "drivers/fb.h"
#include "bin/shell.h"
#include <stdint.h>
#include <string.h>

static uintptr_t kern_int_stack_end;

void ltr(void);

struct tss_entry tss __attribute__((aligned(4096)));

void load_tss(void) {
    asm volatile("mov $kernel_int_stack_end, %0"
                 : "=r"(kern_int_stack_end));

    memset(&tss, 0, sizeof(tss));
    tss.iomap = sizeof(tss);
    tss.ss0 = 0x10;
    tss.esp0 = (uint32_t)kern_int_stack_end;

    tss.cs = 0x8 | 3;
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x10 | 3;

    ltr();
}

void set_kernel_stack(uint32_t stack)
{
   tss.esp0 = stack;
}

