#include "drivers/fb.h"
#include "drivers/int.h"
#include "lib/kernel.h"
#include "int/except_handler.h"
#include <stdint.h>

void gp_fault(struct isr_handler_args i) {
    klog_error("Protection fault\n");
}

void invalid_tss_handler(struct isr_handler_args args) {
    klog_error("Invalid TSS\n");
}

void invalid_opcode(struct isr_handler_args args) {
}

void exception_handlers_init(void) {
    add_isr_handler(13, &gp_fault, 0);
    add_isr_handler(6, &invalid_opcode, 0);
    add_isr_handler(10, &invalid_tss_handler, 0);
}
