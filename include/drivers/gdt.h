#ifndef GDT_H
#define GDT_H

#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t flags;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

#define GDT_SIZE 5

void init_gdt();

void load_gdt();

#endif
