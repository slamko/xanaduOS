#ifndef PROC_H
#define PROC_H

#include "drivers/initrd.h"

int exec_init(void);

void spawn_init(struct module_struct *mods);

int fork(void);

#endif
