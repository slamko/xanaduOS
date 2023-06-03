#include "kernel/syscall.h"
#include "drivers/fb.h"

void syscall_handler(struct isr_handler_args args) {
    fb_newline();
    fb_print_black("syscall");
}
