#include "kernel/syscall.h"

void usermode(void) {
    int res = syscall(1, "Hello\n", 6);

    while(1);
}


