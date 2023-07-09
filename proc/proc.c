#include "proc/proc.h"
#include "drivers/fb.h"
#include "kernel/error.h"
#include "mem/buddy_alloc.h"
#include "mem/frame_allocator.h"
#include "lib/kernel.h"
#include "mem/paging.h"
#include "mem/allocator.h"
#include "fs/fs.h"
#include "lib/kernel.h"
#include "lib/slibc.h"
#include "mem/mmap.h"
#include "drivers/initrd.h"
#include "kernel/syscall.h"

#include <elf.h>
#include <stddef.h>
#include <stdint.h>

void usermode_main(void);

#define USER_STACK_SIZE 4

uintptr_t proc_esp;
uintptr_t user_entry;

struct task {
    struct page_dir *pd;
    uintptr_t eip;
    uintptr_t esp;
};

struct task *task;

int fork(void) {
    int ret;

    task = kmalloc(sizeof(*task));
    task->pd = kzalloc(0, sizeof *task->pd);
    /* klog("FIrst pd addr: %x\n", task->pd->page_tables[0]); */
    
    if (clone_cur_page_dir(task->pd)) {
        klog("error");
        return 1;
    }

    ret = switch_page_dir(task->pd);

    if (ret) {
        klog_error("Switch directory failed\n");
    }

    klog("Switched page dir\n");

    return ret;
}

int execve(const char *exec) {
    int ret = 0;

    struct fs_node *exec_node;
    struct DIR *root_dir = opendir_fs(fs_root);

    for (struct dirent *ent = readdir_fs(root_dir);
         ent;
         ent = readdir_fs(root_dir)) {

        if (strcmp(ent->name, exec) == 0) {
            klog("Execve filename: %s\n", ent->name);
            exec_node = ent->node;
        }
    }

    closedir_fs(root_dir);

    if (!exec_node) {
        return -ENOENT;
    }

    fork();

    /* return 0; */
    /* klog("Modules addr: %x\n", task->pd->page_tables[0]); */

    uintptr_t user_addr[64];
    size_t data_off;

    if (kfsmmap(exec_node, user_addr, &data_off, USER | R_W | PRESENT)) {
        panic("Failed to map init executable into memory\n", ENOMEM);
    }


    Elf32_Ehdr *elf = (Elf32_Ehdr *)(*user_addr + data_off);
    uintptr_t user_eip = elf->e_entry;
    user_entry = *user_addr + data_off + user_eip + 0x100A;

    uintptr_t user_esp[USER_STACK_SIZE];
    knmmap(cur_pd, user_esp, 0, USER_STACK_SIZE, USER | R_W | PRESENT); 
    user_esp[0] += (USER_STACK_SIZE - 1) * 0x1000;

    klog("User process stack pointer %x\n", user_esp[0]);
    klog("\nElf entry point %x\n", user_entry);

    for (unsigned int i = 0; i < 0x100; i++) {
        fb_print_hex(*(uint8_t *)(user_entry + i));
    }

    jump_usermode(user_entry, *user_esp);
    return ret;
}

void spawn_init(struct module_struct *mods) {
    initrd_init(mods, fs_root);
    
    fs_root = initrd_get_root();

    if (execve("init")) {
        panic("Failed to launch init process\n", 0);
    }

    return;
}
