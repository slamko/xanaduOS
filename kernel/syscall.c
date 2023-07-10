#include "kernel/syscall.h"
#include "drivers/fb.h"
#include "drivers/int.h"
#include "drivers/keyboard.h"
#include "lib/kernel.h"
#include "proc/proc.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

const unsigned int SYSCALL_MAX_ARGS_NUM = 5;

void syscall_setup(void);
void scall_wrapper(struct isr_handler_args *args);

typedef int (*syscall_f)(va_list args);

#define SYSCALL_DEFINE0(name)                                                  \
    int STRCAT(name, _v)(va_list args) {                                       \
        (void)args;                                                            \
        int ret = name();                                                      \
        return ret;                                                            \
    }

#define SYSCALL_DEFINE1(name, type1)                                           \
    int STRCAT(name, _v)(va_list args) {                                       \
        va_list sc_args;                                                       \
        va_copy(sc_args, args);                                                \
        type1 STRCAT(arg, 1) = va_arg(sc_args, type1);                         \
                                                                               \
        int ret = name(STRCAT(arg, 1));                                        \
        va_end(sc_args);                                                       \
        return ret;                                                            \
    }

#define SYSCALL_DEFINE2(name, type1, type2)                                    \
    int STRCAT(name, _v)(va_list args) {                                       \
        va_list sc_args;                                                       \
        va_copy(sc_args, args);                                                \
        type1 STRCAT(arg, 1) = va_arg(sc_args, type1);                         \
        type2 STRCAT(arg, 2) = va_arg(sc_args, type2);                         \
                                                                               \
        int ret = name(STRCAT(arg, 1), STRCAT(arg, 2));                        \
        va_end(sc_args);                                                       \
        return ret;                                                            \
    }

#define SYSCALL_DEFINE3(name, type1, type2, type3)                             \
    int STRCAT(name, _v)(va_list args) {                                       \
        va_list sc_args;                                                       \
        va_copy(sc_args, args);                                                \
        type1 STRCAT(arg, 1) = va_arg(sc_args, type1);                         \
        type2 STRCAT(arg, 2) = va_arg(sc_args, type2);                         \
        type3 STRCAT(arg, 3) = va_arg(sc_args, type2);                         \
                                                                               \
        int ret = name(STRCAT(arg, 1), STRCAT(arg, 2), STRCAT(arg, 3));        \
        va_end(sc_args);                                                       \
        return ret;                                                            \
    }

SYSCALL_DEFINE2(sys_write, const char *, size_t);

SYSCALL_DEFINE2(sys_read, void *, size_t);

SYSCALL_DEFINE0(sys_fork);

SYSCALL_DEFINE1(sys_exit, int);

SYSCALL_DEFINE1(sys_execve, const char *);

syscall_f syscall_table[] = {
    [0]   = &sys_read_v,
    [1]   = &sys_write_v,
    [2]   = &sys_fork_v,
    [59]  = &sys_execve_v,
    [60]  = &sys_exit_v,
};

int sys_write(const char *msg, size_t len) {
    fb_nprint_black(msg, len);
    return len;
}

int sys_exit(int code) {
    exit(code);
    return 0;
}

int sys_fork(void) { return fork(); }

int sys_read(void *buf, size_t count) {
    uint32_t *read_buf = (uint32_t *)buf;
    size_t i;

    for (i = 0; i < count; i++) {
        read_buf[i] = kbd_read();
    }
    return i;
}

int sys_execve(const char *exec_name) {
    return execve(exec_name);
}

int sys_echo(int len) {
    fb_print_num(len);
    fb_newline();
    return len;
}

int syscall_exec(int num, ...) {
    va_list args;
    va_start(args, num);

    int ret = syscall_table[num](args);

    /* klog("Syscall return status: %d\n", ret); */
    va_end(args);
    return ret;
}

void scall_s(struct isr_handler_args *a) { klog("Int 0x80 %x\n", a->esp); }

void syscall_init(void) {
    add_isr_handler(0x80, &scall_wrapper, IDTD_RING3 | IDTD_DEFAULT);
    syscall_setup();
}
