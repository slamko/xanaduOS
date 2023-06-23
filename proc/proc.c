#include "proc/proc.h"
#include "drivers/fb.h"
#include "mem/frame_allocator.h"
#include "lib/kernel.h"
#include "mem/paging.h"
#include "mem/allocator.h"
#include <stdint.h>

void usermode_main(void);

static uintptr_t usermode_text_start;
static uintptr_t usermode_text_end;

int exec_init(void) {
    int ret = 0;
    struct page_dir new_pd;
    
    if (clone_cur_page_dir(&new_pd)) {
        klog("error");
        return 1;
    }

    ret = switch_page_dir(&new_pd);
    klog("switch");

    asm volatile ("mov $_usermode_text_start, %0" : "=r"(usermode_text_start));
    asm volatile ("mov $_usermode_text_end, %0" : "=r"(usermode_text_end));

    uint16_t pde, pte; 
    uint16_t end_pde, end_pte; 
    get_pde_pte(usermode_text_start, &pde, &pte);
    get_pde_pte(usermode_text_end, &end_pde, &end_pte);

    fb_print_hex(usermode_text_start);
    fb_print_hex(usermode_text_end);
    
    page_table_t user_pt;
    new_pd.page_tables[pde] = alloc_pt(&user_pt, USER | R_W | PRESENT);

    for (unsigned int i = pte; i <= end_pte + 1; i++) {
        user_pt[pte] = alloc_frame(((uintptr_t)pde << 22) & ((uintptr_t)pte << 12), USER | R_W | PRESENT);
    }

    asm volatile ("mov %0, %%esp"
                  :
                  : "r" (((uintptr_t)pde << 22) & ((uintptr_t)(end_pde + 1) << 12)));

    usermode_main();
    
    return ret;
}
