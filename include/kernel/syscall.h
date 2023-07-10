#ifndef SYSCALL_H
#define SYSCALL_H

#include "drivers/int.h"
#include <stddef.h>
#include <stdint.h>

extern const unsigned int SYSCALL_MAX_ARGS_NUM;

#define _STRCAT(a, b) a ## b 
#define STRCAT(a, b) _STRCAT(a, b)

enum {
    SYS_READ = 0,
    SYS_WRITE = 1,
};

void jump_usermode(uintptr_t eip, uintptr_t esp);

int syscall_handler(int ecx, int edx, unsigned int num, ...);
int syscall(unsigned int num, ...);

int sys_write(const char *msg, size_t len);

int sys_read(void *buf, size_t count);

int sys_fork(void);

void syscall_init(void);

int sys_exit(int code);

int sys_execve(const char *exec_name);

#endif
