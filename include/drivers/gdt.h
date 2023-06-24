#ifndef GDT_H
#define GDT_H

#include <stdint.h>

enum ACCESS_BITS {
    GDTD_PRESENT        = 7,
    GDTD_DPL            = 5,
    GDTD_NON_SYSTEM_SEG = 4,
    GDTD_EXEC           = 3,
    GDTD_DC             = 2,
    GDTD_RW             = 1,
    GDTD_ACCESSED       = 0,
};

enum DESCRIPTOR_ACCESS {
    GDTD_PRESENT_MASK           = (1 << 7),
    GDTD_DPL_MASK               = (3 << 5),
    GDTD_NON_SYSTEM_SEG_MASK    = (1 << 4),
    GDTD_EXEC_MASK              = (1 << 3),
    GDTD_DC_MASK                = (1 << 2),
    GDTD_RW_MASK                = (1 << 1),
    GDTD_ACCESSED_MASK          = (1 << 0),
};

enum DESCRIPTOR_FLAGS {
    GDTF_GRAN           = (1 << 3),
    GDTF_PROTECTED_MODE = (1 << 2),
    GDTF_LONG_MODE      = (1 << 1),
};

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t acces;
    uint8_t limit_high : 4;
    uint8_t flags : 4;
    int8_t base_high;
} __attribute__((packed));

struct gdt_entry_bits {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;

    union {
        struct {
            uint8_t accessed : 1;
            uint8_t rw : 1;
            uint8_t direction : 1;
            uint8_t exec : 1;
        };

        uint8_t type : 4;
    };
    
    uint8_t non_sys : 1;
    uint8_t dpl : 2;
    uint8_t present : 1;
    
    uint8_t limit_high : 4;
    uint8_t reserved : 1;
    uint8_t long_mode : 1;
    uint8_t db_size : 1;
    uint8_t granularity : 1;
    uint8_t base_high;
} __attribute__((packed));

struct gdtr {
    uint16_t limit;
    uintptr_t base;
} __attribute__((packed));

#define GDT_SIZE 6

void gdt_init(void);

void load_gdt(uintptr_t gdt_ptr);

#endif
