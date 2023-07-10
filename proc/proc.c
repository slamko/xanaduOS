#include "proc/proc.h"
#include "drivers/fb.h"
#include "drivers/int.h"
#include "drivers/pit.h"
#include "kernel/error.h"
#include "mem/buddy_alloc.h"
#include "mem/slab_allocator.h"
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

struct slab_cache *task_slab;

size_t last_kernel_tid;
size_t last_user_tid;

enum task_state {
    EMPTY           = 0,
    RUNNING         = 1,
    IDLE            = 2,
    DEAD            = 3,
};

struct task {
    struct page_dir *pd;
    uintptr_t eip;
    uintptr_t esp;
    uintptr_t *map_addr;
    size_t map_page_num;
    enum task_state state;

    size_t id;
    struct buddy_alloc *buddy;
    struct fs_node *exec_node;
    struct task *next;
    struct task *prev;
};

struct task *task_list;
struct task *cur_task;

int load_elf(struct task *task, struct fs_node *exec_node) {
    uintptr_t *user_addr;
    size_t map_npages = page_align_up(exec_node->size);
    user_addr = kmalloc(map_npages);
    
    size_t data_off;

    if (kfsmmap(task->buddy, exec_node, user_addr, &data_off,
                USER | R_W | PRESENT)) {
        panic("Failed to map init executable into memory\n", ENOMEM);
    }

    Elf32_Ehdr *elf = (Elf32_Ehdr *)(*user_addr + data_off);
    uintptr_t user_eip = elf->e_entry;
    uintptr_t user_entry = *user_addr + data_off + user_eip + 0x100A;

    uintptr_t user_esp[USER_STACK_SIZE];
    knmmap(task->buddy, cur_pd, user_esp, 0, USER_STACK_SIZE,
           USER | R_W | PRESENT); 
    user_esp[0] += (USER_STACK_SIZE - 1) * 0x1000;

    task->eip = user_entry;
    task->esp = user_esp[0];
    task->map_addr = user_addr;
    task->map_page_num = map_npages;

    klog("User process stack pointer %x\n", user_esp[0]);
    klog("\nElf entry point %x\n", user_entry);

    for (unsigned int i = 0; i < 0x100; i++) {
        fb_print_hex(*(uint8_t *)(user_entry + i));
    }

    return 0;
}

void run_user_task(struct task *task) {
    cur_task = task;
    jump_usermode(task->eip, task->esp);
}

void run_kernel_task(struct task *task) {
    klog("Kernel again\n");

    while(1);
}

void schedule(void) {
    struct task *task = task_list;

    foreach(task,
            if (task->state == EMPTY) {
                task->state = RUNNING;
                load_elf(task, task->exec_node);
                run_user_task(task);
            }
        );
}

void kern() {
    klog("Again in kernel \n");

    while(1);
}

void free_task(struct task *task) {
    free_pd(task->pd);
    /* knmunmap(task->pd, task->map_addr, task->map_page_num); */
    slab_free(task_slab, task);
}

static inline void switch_task(struct task *switch_task) {
    switch_page_dir(switch_task->pd);
    klog("Switching task\n");

    if (switch_task->id >= 1000) {
        run_user_task(switch_task);
    } else {
        run_kernel_task(switch_task);
    }
}

void exit(int code) {
    doubly_ll_remove(&task_list, cur_task);

    if (!task_list) {
        panic("Killed kernel task...\n", 0);
    }

    free_task(cur_task);
    switch_task(task_list);
}

int fork(void) {
    return 0;
}

int fork_task(struct task *new_task) {
    int ret;

    memcpy(new_task, cur_task, sizeof(*new_task));
    new_task->pd = kzalloc(0, sizeof *new_task->pd);
    new_task->id = last_kernel_tid + 1;

    if (cur_task->id >= 1000) {
        cur_task->id = last_user_tid + 1;
    }
   
    if (clone_cur_page_dir(new_task->pd)) {
        klog("error");
        return 1;
    }

    ret = switch_page_dir(new_task->pd);

    if (ret) {
        klog_error("Switch directory failed\n");
    }

    klog("Switched page dir\n");

   return ret;
}

int execve(const char *exec) {
    int ret = 0;
    struct fs_node *exec_node = root_get_node_fs(exec);

    if (!exec_node) {
        return -ENOENT;
    }

    cur_task->exec_node = exec_node;
    cur_task->state = EMPTY;
    cur_task->buddy = buddy_alloc_create(0x100000, 0x40000000);
    
    return ret;
}

void spawn_init(struct module_struct *mods) {
    initrd_init(mods, fs_root);
    
    fs_root = initrd_get_root();
    uint32_t eflags = disable_int();

    struct fs_node *exec_node = root_get_node_fs("init");
    
    struct task *new_task = slab_alloc_from_cache(task_slab);

    if (fork_task(new_task)) {
        panic("Failed to fork base kernel process\n", 0);
    }

    new_task->id = last_user_tid;
    new_task->state = EMPTY;
    new_task->exec_node = exec_node;
    last_user_tid++;

    new_task->buddy = buddy_alloc_create(0x100000, 0x40000000);
    doubly_ll_insert(task_list, new_task);
    recover_int(eflags);

    return;
}

int multiproc_init(void) {
    task_slab = slab_cache_create(sizeof(struct task));
    pit_add_callback(&schedule, 0, 8);
    cur_task = slab_alloc_from_cache(task_slab);
    cur_task->pd = kernel_pd;
    cur_task->buddy = kern_buddy;
    cur_task->eip = to_uintptr(&kern);
    cur_task->id = 0;
    cur_task->state = RUNNING;

    last_kernel_tid = 0;
    last_user_tid = 1000;
    doubly_ll_insert(task_list, cur_task);
    
    return 0;
}
