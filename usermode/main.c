#include "kernel/syscall.h"

int usr_sysenter(unsigned int num, uintptr_t func, ...);

int main(void) {
    int res = 0;

    res = usr_sysenter(1, 2, "Hello\n", 6);
    res = usr_sysenter(1, 2, "Hello\n", 6);

    while(1);
    return 0;
}


