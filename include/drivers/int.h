#ifndef INT_H
#define INT_H

#include <stdint.h>

#define SYSCALL_INT 0x80

enum IRQ_Ids {
    KBD_IRQ  = 1,
    COM2_IRQ = 3,
    COM1_IRQ = 4,
    SYSCALL  = 128
};

enum BASE_IRQ {
    KBD_MASK    = (1 << KBD_IRQ),
    COM2_MASK   = (1 << COM2_IRQ),
    COM1_MASK   = (1 << COM1_IRQ),
};

enum IDT_DESCRIPTOR_FLAGS {
    IDTD_PRESENT        = (1 << 7),
    IDTD_PROTECTED_MODE = (1 << 3),
};

enum RESERVED_INTERRUPTS {
    DIV_BY_ZERO_INT = 0x00,
    SSI             = 0x01,
    NMI             = 0x02,
    OVERFLOW_INT    = 0x04,
    DOUBLE_F_INT    = 0x08,
    GP_INT          = 0x0D,
    PAGE_F_INT      = 0x0E,
};

#define INT_GATE_MASK (0x6)

struct idt_entry {
    uint16_t isr_low;
    uint16_t cs;
    uint8_t reserved;
    uint8_t attributes;
    uint16_t isr_high;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct x86_cpu_state {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
} __attribute__((packed));

struct isr_stack {
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} __attribute__((packed));

struct isr_full_stack {
    struct x86_cpu_state cpu_st;
    uint32_t int_num;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} __attribute__((packed));

struct isr_handler_args {
    uint32_t int_id;
};

static inline void cli(void) {
    asm volatile ("cli");
}

void init_idt();

static inline void sti(void) {
    asm volatile ("sti");
}

/* void isr_x86(struct x86_cpu_state, uint32_t int_num, struct isr_stack);  */
void isr_x86 (struct isr_full_stack);

#endif
