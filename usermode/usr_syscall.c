#include <stdint.h>
#include <stdarg.h>
#include "kernel/syscall.h"

int usr_sysenter(unsigned int num, uintptr_t func, ...);

#define SYSCALL0(num)                          \
    usr_sysenter(func, 0, 0, 0, 0, 0);

#define SYSCALL1(num, ret, args, type)                                   \
    ret = usr_sysenter(num, 1, (uintptr_t)&func, va_arg(args, type), 0, 0, 0, 0);

#define SYSCALL2(num, ret, type1, type2)  ;               \
    type1 STRCAT(func, 1) = va_arg(args, type1);        \
    type2 STRCAT(func, 2) = va_arg(args, type2);        \
    ret = usr_sysenter(num, 2, STRCAT(func, 1), STRCAT(func, 2), 0, 0, 0);
 
#define SYSCALL3(num, ret, args, type1, type2, type3)  ;    \
    type1 STRCAT(func, 1) = va_arg(args, type1);        \
    type2 STRCAT(func, 2) = va_arg(args, type2);        \
    type3 STRCAT(func, 3) = va_arg(args, type3);        \
    ret = usr_sysenter(num, 3, (uintptr_t)&func,             \
                       STRCAT(func, 1), STRCAT(func, 2),    \
             STRCAT(func, 3), 0, 0);

#define SYSCALL4(num, ret, args, type1, type2, type3, type4);           \
    type1 STRCAT(func, 1) = va_arg(args, type1);                        \
    type2 STRCAT(func, 2) = va_arg(args, type2);                        \
    type3 STRCAT(func, 3) = va_arg(args, type3);        \
    type4 STRCAT(func, 4) = va_arg(args, type4);        \
    usr_sysenter(num, 4, STRCAT(func, 1), STRCAT(func, 2),   \
STRCAT(func, 3), STRCAT(func, 4), 0);

#define SYSCALL5(func, args, type1, type2, type3, type4, type5)       \
    type1 STRCAT(func, 1) = va_arg(args, type1);                        \
    type2 STRCAT(func, 2) = va_arg(args, type2);                        \
    type3 STRCAT(func, 3) = va_arg(args, type3);        \
    type4 STRCAT(func, 4) = va_arg(args, type4);        \
    type4 STRCAT(func, 5) = va_arg(args, type4);        \
    ret = usr_sysenter(num, 5, STRCAT(func, 1), STRCAT(func, 2),  \
STRCAT(func, 3), STRCAT(func, 4), 0);

int syscall(unsigned int num, ...) {
    va_list args;
    va_start(args, num);

    int ret;

    SYSCALL2(num, ret, const char *, size_t);

    va_end(args);
    return ret;
}

