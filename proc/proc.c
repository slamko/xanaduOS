#include "proc/proc.h"
#include "drivers/fb.h"
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

static uintptr_t usermode_text_start;
static uintptr_t usermode_text_end;

struct buddy_alloc *user_buddy;

#define USER_STACK_SIZE 4

uintptr_t proc_esp;

int exec_init(void) {
    int ret = 0;
    struct page_dir new_pd;
    
    if (clone_cur_page_dir(&new_pd)) {
        klog("error");
        return 1;
    }

    ret = switch_page_dir(&new_pd);

    __asm__ volatile ("mov $_usermode_text_start, %0" : "=r"(usermode_text_start));
    __asm__ volatile ("mov $_usermode_text_end, %0" : "=r"(usermode_text_end));

    uint16_t pde, pte; 
    uint16_t end_pde, end_pte; 
    get_pde_pte(usermode_text_start, &pde, &pte);
    get_pde_pte(usermode_text_end, &end_pde, &end_pte);

    fb_print_hex(pde);
    fb_print_hex(pte);
    
    page_table_t user_pt;
    new_pd.page_tables[pde] = alloc_pt(&user_pt, USER | R_W | PRESENT);

    uintptr_t frame_addr = get_ident_phys_page_addr(pde, pte);
    size_t frames_n = end_pte - pte + 2;

    if (alloc_nframes(frames_n, frame_addr, &user_pt[pte], USER | R_W | PRESENT)) {
        return 1;
    }

    fb_print_num(frames_n);
    for (unsigned int i = pte; i <= end_pte + 1u; i++) {
        fb_print_hex(user_pt[i]);
    }
    proc_esp = get_ident_phys_page_addr(pde, end_pte + 1);
    
    return ret;
}

void usermode() {
    while(1);
}

void spawn_init(struct module_struct *mods) {
    initrd_init(mods, fs_root);
    
    fs_root = initrd_get_root();
    struct fs_node *user_main;

    struct DIR *root_dir = opendir_fs(fs_root);

    for (struct dirent *ent = readdir_fs(root_dir);
         ent;
         ent = readdir_fs(root_dir)) {

        if (strcmp(ent->name, "init") == 0) {
            klog("Initrd filename: %s\n", ent->name);
            user_main = ent->node;
        }
    }

    closedir_fs(root_dir);

    uintptr_t user_addr[64];
    size_t data_off;

    if (kfsmmap(user_main, user_addr, &data_off, USER | R_W | PRESENT)) {
        klog_error("Failed to map init executable into memory\n");
    }

    klog("Modules addr: %x\n", mods->mod_start);
    fb_print_black((char *)(*user_addr + data_off));

    Elf32_Ehdr *elf = (Elf32_Ehdr *)(*user_addr + data_off);
    uintptr_t user_eip = elf->e_entry;
    uintptr_t user_entry = *user_addr + data_off + user_eip + 0x100A;

    uintptr_t user_esp[USER_STACK_SIZE];
    knmmap(cur_pd, user_esp, 0, USER_STACK_SIZE, USER | R_W | PRESENT); 
    user_esp[0] += (USER_STACK_SIZE - 1) * 0x1000;
    klog("User stack pointer %x\n", user_esp[0]);

    for (unsigned int i = 0; i < 0x100; i++) {
        /* fb_print_hex(*(uint8_t *)(user_entry + i)); */
    }
 
    klog("\nElf entry point %x\n", user_entry);

    flush_tlb();
    /* *(char *)(void *)user_esp[0] = 'a'; */
    /* fb_putc(*(char *)(void *)user_esp[0]); */
    jump_usermode(user_entry, *user_esp);
}
