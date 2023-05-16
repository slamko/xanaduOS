#include "mem/paging.h"

extern void enable_paging();

void paging_init() {
    enable_paging();
}
