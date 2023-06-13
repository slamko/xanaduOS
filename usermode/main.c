#include "kernel/syscall.h"

void usermode(void) {
    syscall(1, "Hello\n", 6);
    syscall(1, "Hello\n", 6);
    
    while(1);
}


