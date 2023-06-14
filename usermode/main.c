#include "kernel/syscall.h"

void usermode(void) {
    int res = syscall(1, "Hello\n", 6);
    char some[] = {res + '0'};
    syscall(1, some, 1);

    res = syscall(1, "Hello\n", 6);
    char new[] = {res + '0'};
    syscall(1, new, 1);

    while(1);
}


