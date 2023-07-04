#include "proc/proc.h"
#include "drivers/fb.h"
#include "mem/frame_allocator.h"
#include "lib/kernel.h"
#include "mem/paging.h"
#include "mem/allocator.h"
#include <stddef.h>
#include <stdint.h>

void usermode_main(void);

static uintptr_t usermode_text_start;
static uintptr_t usermode_text_end;

void jump_usermode(void);

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
