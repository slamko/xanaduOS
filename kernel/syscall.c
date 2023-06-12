#include "kernel/syscall.h"
#include "drivers/fb.h"

void syscall_setup(void);

void syscall_handler(void) {
    fb_newline();
    fb_print_black("syscall");
}

void syscall_init(void) {
    syscall_setup();
}
