#include "kernel/syscall.h"

void usermode(void) {
    /* asm volatile ("xchgw %bx, %bx"); */
    int res = syscall(1, "Hello\n", 6);

    /* asm volatile ("int $0x33"); */
    /* char some[] = {res + '0'}; */
    /* syscall(1, some, 1); */
    /* asm volatile ("mov $0x9E, %ecx;"); */
    asm volatile ("cli");

    res = syscall(1, "Hello\n", 6);
    /* char new[] = {res + '0'}; */
    /* syscall(1, new, 1); */

    while(1);
}


