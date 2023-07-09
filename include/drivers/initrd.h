#ifndef INITRD_H
#define INITRD_H

#include <stdint.h>
#include "fs/fs.h"

struct module_struct {
    uintptr_t mod_start;
    uintptr_t mod_end;
    char *string;
    int reserved;
} __attribute__((packed));

struct fs_node *initrd_get_root(void);

int initrd_init(struct module_struct *modules, struct fs_node *root);

void print_header(int inode);
#endif
