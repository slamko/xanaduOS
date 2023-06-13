#include "kernel/syscall.h"
#include <stdarg.h>
#include <stddef.h>
#include "drivers/fb.h"

void syscall_setup(void);

__attribute__((regparm(0))) void sysenter_call();

int sys_write(const char *msg, size_t len) {
    fb_nprint_black(msg, len);
    return len;
}

int syscall_handler(int edx, int ecx, unsigned int num, ...) {
    va_list args;
    va_start(args, num);
    fb_print_num(num);


    switch (num) {
    case SYS_READ:
        break;
    case SYS_WRITE:
    {
        fb_print_black("os\n");
        /* const char *msg = va_arg(args, const char *); */
        /* size_t len = va_arg(args, size_t); */
        /* sys_write(msg, len); */

        break;
    }
    }

    va_end(args);
    return 0;
}

void syscall_init(void) {
    syscall_setup();
}
