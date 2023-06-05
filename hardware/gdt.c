#include "drivers/gdt.h"
#include "drivers/fb.h"
#include "drivers/int.h"
#include "drivers/tss.h"
#include "lib/slibc.h"
#include <stdint.h>

static struct gdt_entry gdt[GDT_SIZE];
static struct gdtr gdtr;

void gdt_fill_entry(int32_t id, uint32_t base, uint32_t limit,
                    uint8_t access, uint8_t flags) {
    struct gdt_entry *entry = &gdt[id];
  
    entry->base_low = base & 0xffff;
    entry->base_middle = (base >> 16) & 0xff;
    entry->base_high = (base >> 24) & 0xff;

    entry->limit_low = limit & 0xffff;
    entry->limit_high = (limit >> 16) & 0x0f;

    entry->acces = access;
    entry->flags = flags;
} 

void fill_gdt() {
    gdtr.base = (uintptr_t)&gdt;
    gdtr.limit = sizeof(gdt) - 1;

    gdt_fill_entry(0, 0, 0, 0, 0);                
    gdt_fill_entry(1, 0,
                   UINTPTR_MAX,
                   GDTD_PRESENT_MASK |
                   GDTD_NON_SYSTEM_SEG_MASK |
                   GDTD_EXEC_MASK |
                   GDTD_RW_MASK,
                   GDTF_GRAN | GDTF_PROTECTED_MODE); 

    gdt_fill_entry(2, 0, UINTPTR_MAX,
                   GDTD_PRESENT_MASK |
                   GDTD_NON_SYSTEM_SEG_MASK |
                   GDTD_RW_MASK,
                   GDTF_GRAN | GDTF_PROTECTED_MODE); 

    gdt_fill_entry(3, 0, UINTPTR_MAX,
                   GDTD_PRESENT_MASK |
                   GDTD_NON_SYSTEM_SEG_MASK |
                   GDTD_DPL_MASK |
                   GDTD_EXEC_MASK |
                   GDTD_RW_MASK,
                   GDTF_GRAN | GDTF_PROTECTED_MODE);

    gdt_fill_entry(4, 0, UINTPTR_MAX,
                   GDTD_PRESENT_MASK |
                   GDTD_NON_SYSTEM_SEG_MASK |
                   GDTD_DPL_MASK |
                   GDTD_RW_MASK,
                   GDTF_GRAN | GDTF_PROTECTED_MODE);

    gdt_fill_entry(5, (uintptr_t)&tss,  (uintptr_t)&tss + sizeof(tss) - 1,
                   GDTD_ACCESSED_MASK |
                   GDTD_EXEC_MASK |
                   GDTD_DPL_MASK |
                   GDTD_PRESENT_MASK,
                   0x00);

    load_gdt((uintptr_t)&gdtr);
    load_tss();
}

void init_gdt() {
    cli();
    fill_gdt();
    fb_print_black("Setting up GDT\n");
}
        

