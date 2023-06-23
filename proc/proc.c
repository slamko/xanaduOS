#include "mem/paging.h"
#include "mem/allocator.h"
#include "proc/proc.h"
#include <stdint.h>

int exec_init(void) {
    int ret = 0;
    struct page_dir new_pd;
    
    if (clone_cur_page_dir(&new_pd)) {
        return 1;
    }

    ret = switch_page_dir(&new_pd);
    
    return ret;
}
