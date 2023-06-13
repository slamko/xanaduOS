#include "drivers/int.h"

enum {
    SYS_READ = 0,
    SYS_WRITE = 1,
};

int syscall_handler(int ecx, int edx, unsigned int num, ...);

void syscall_init(void);
