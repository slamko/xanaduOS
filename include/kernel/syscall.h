#include "drivers/int.h"

extern const unsigned int SYSCALL_MAX_ARGS_NUM;

enum {
    SYS_READ = 0,
    SYS_WRITE = 1,
};

int syscall_handler(int ecx, int edx, unsigned int num, ...);
int syscall(unsigned int num, ...);

void syscall_init(void);
