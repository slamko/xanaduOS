#include "kernel/syscall.h"
#include <stdint.h>

int usr_sysenter(unsigned int num, unsigned int arg_num, ...);

int usr_intx80(unsigned int num, unsigned int arg_num, ...);

int userloop(void) {

    while(1);
}

int main(void) {
    int res = 0;

    /* __asm__ volatile("int $0x80"); */
    res = usr_intx80(1, 2, "Hello\n", 6);
    /* res = usr_intx80(1, 2, "Hello\n", 6); */
    /* res = usr_intx80(1, 2, "Hello\n", 6); */
    /* res = usr_intx80(1, 2, "Hello\n", 6); */

    /* res = usr_intx80(59, 1, "shell"); */
    if (res) {

        /* res = usr_intx80(1, 2, "Hello\n", 6); */
    }
    /* usr_intx80(2, 1, 0); */

    /* res = usr_sysenter(1, 2, "Hello\n", 6); */
    while(1);

    /* usr_intx80(60, 1, 0); */
    
    return res;
}


