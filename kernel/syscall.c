#include "kernel/syscall.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "drivers/fb.h"
#include "drivers/keyboard.h"

void syscall_setup(void);

void sysenter(void *, ...);

__attribute__((regparm(0))) void sysenter_call();

int sys_write(const char *msg, size_t len) {
    fb_nprint_black(msg, len);
    return len;
}

int sys_read(void *buf, size_t count) {
    uint32_t *read_buf = (uint32_t *)buf;
    size_t i;

    for (i = 0; i < count; i++) {
        read_buf[i] = kbd_read();
    }
    return i != count;
}

int sys_echo(int len) {
    fb_print_num(len);
    fb_newline();
    return len;
}

int syscall(unsigned int num, ...) {
    va_list args;
    va_start(args, num);
    /* fb_print_num(num); */

    switch (num) {
    case SYS_READ:
        break;
    case SYS_WRITE:
    {
        const char *msg = va_arg(args, const char *);
        size_t len = va_arg(args, size_t);
        sysenter(&sys_write, msg, len, 1, 2, 4);

        break;
    }
    }

    va_end(args);
    return 0;
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
