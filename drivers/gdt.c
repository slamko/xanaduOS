#include "drivers/gdt.h"
#include <stdint.h>

struct gdt_entry gdt[GDT_SIZE];
struct gdtr gdtr;

void gdt_fill_entry(int32_t id, uint32_t base, uint32_t limit, uint8_t flags, uint8_t gran) {
    gdt[id].base_low = base & 0xffff;
    gdt[id].base_middle = (base >> 16) & 0xff;
    gdt[id].base_middle = (base >> 24) & 0xff;

    gdt[id].limit_low = limit & 0xffff;
    gdt[id].granularity = (limit >> 16) & 0x0f;
    gdt[id].granularity |= gran & 0xf0;
    gdt[id].flags = flags;
} 

void fill_gdt() {
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint32_t)gdt;

    gdt_fill_entry(0, 0, 0, 0, 0);                // Null segment
    gdt_fill_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdt_fill_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    gdt_fill_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
    gdt_fill_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

    load_gdt((uint32_t)&gdtr);
}

