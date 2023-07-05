#include "kernel/syscall.h"

void usermode(void) {
    int res = 0;

    res = syscall(1, "Hello\n", 6);
    res = syscall(1, "Hello\n", 6);

    while(1);
}


