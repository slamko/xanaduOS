#ifndef INITRD_H
#define INITRD_H

#include <stdint.h>

struct module_struct {
    uintptr_t mod_start;
    uintptr_t mod_end;
    char *string;
    int reserved;
} __attribute__((packed));


int initrd_init(struct module_struct *modules);

#endif
