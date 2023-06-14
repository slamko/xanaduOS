#include "kernel/syscall.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "drivers/fb.h"
#include "drivers/keyboard.h"

const unsigned int SYSCALL_MAX_ARGS_NUM = 5;

void syscall_setup(void);

int sysenter(unsigned int num, uintptr_t func, ...);

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
    return i;
}

int sys_echo(int len) {
    fb_print_num(len);
    fb_newline();
    return len;
}

#define _STRCAT(a, b) a ## b 
#define STRCAT(a, b) _STRCAT(a, b)

#define SYSCALL0(func)                               \
    sysenter(func, 0, 0, 0, 0, 0);

#define SYSCALL1(func, args, type)                   \
    sysenter(func, va_arg(args, type), 0, 0, 0, 0);

#define SYSCALL2(func, ret, type1, type2)  ;               \
    type1 STRCAT(func, 1) = va_arg(args, type1);        \
    type2 STRCAT(func, 2) = va_arg(args, type2);        \
    ret = sysenter(2, (uintptr_t)&func, STRCAT(func, 1), STRCAT(func, 2), 0, 0, 0);
 
#define SYSCALL3(func, args, type1, type2, type3)  ;        \
    type1 STRCAT(func, 1) = va_arg(args, type1);        \
    type2 STRCAT(func, 2) = va_arg(args, type2);        \
    type3 STRCAT(func, 3) = va_arg(args, type3);        \
    sysenter(3, (uintptr_t)&func, STRCAT(func, 1), STRCAT(func, 2),  \
             STRCAT(func, 3), 0, 0);

#define SYSCALL4(func, args, type1, type2, type3, type4); \
    type1 STRCAT(func, 1) = va_arg(args, type1);                        \
    type2 STRCAT(func, 2) = va_arg(args, type2);                        \
    type3 STRCAT(func, 3) = va_arg(args, type3);        \
    type4 STRCAT(func, 4) = va_arg(args, type4);        \
    sysenter(func, STRCAT(func, 1), STRCAT(func, 2),    \
STRCAT(func, 3), STRCAT(func, 4), 0);

#define SYSCALL5(func, args, type1, type2, type3, type4, type5)       \
    sysenter(func,                                                    \
             va_arg(args, type1),                                    \
             va_arg(args, type2),                                   \
             va_arg(args, type3),                                   \
             va_arg(args, type4),                                   \
             va_arg(args, type5),                                   \
             0);

int syscall(unsigned int num, ...) {
    va_list args;
    va_start(args, num);

    int ret;

    switch (num) {
    case SYS_READ:
        SYSCALL2(sys_read, ret, void *, size_t);
        break;
    case SYS_WRITE:
        SYSCALL2(sys_write, ret, const char *, size_t);
        break;
    }

    va_end(args);
    return ret;
}

void syscall_init(void) {
    syscall_setup();
}
