#include "drivers/fb.h"
#include "drivers/int.h"
#include "drivers/pic.h"
#include "mem/paging.h"
#include "lib/kernel.h"
#include "int/except_handler.h"
#include <stdint.h>

static uintptr_t last_eip;

void gp_fault(struct isr_handler_args i) {
    klog_error("Protection fault\n");
}

void invalid_tss_handler(struct isr_handler_args args) {
    klog_error("Invalid TSS\n");
}

void invalid_opcode(struct isr_handler_args args) {
    if (last_eip == args.eip) return;

    last_eip = args.eip;
    klog_error("Invalid opcode\n");
}

static void spurious_handler(struct isr_handler_args args) {
    klog_error("Spurious interrupr");
}

void exception_handlers_init(void) {
    add_isr_handler(6, &invalid_opcode, 0);
    add_isr_handler(7, &spurious_handler, 0);
    add_isr_handler(10, &invalid_tss_handler, 0);
    add_isr_handler(13, &gp_fault, 0);
    add_isr_handler(14, &page_fault, 0);
}
