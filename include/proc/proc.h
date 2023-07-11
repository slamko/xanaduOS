#ifndef PROC_H
#define PROC_H

#include "drivers/initrd.h"

struct task;

int exec_init(void);

void spawn_init(struct module_struct *mods);

int multiproc_init(void);

int fork(void);

int fork_task(struct task *new_task, int i);

void exit(int code);

int execve(const char *exec);

#endif
