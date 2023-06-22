#include "mem/paging.h"
#include "mem/allocator.h"
#include "proc/proc.h"
#include <stdint.h>

int exec_init(void) {
    uintptr_t *pd = clone_page_dir(NULL);

    if (!pd) {
        return 1;
    }

    
    
}
