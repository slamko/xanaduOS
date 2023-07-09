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

#define INT_GATE_MASK (0xE)

enum IDT_DESCRIPTOR_FLAGS {
    IDTD_PRESENT        = (1 << 7),
    IDTD_PROTECTED_MODE = (1 << 3),
    IDTD_RING3          = (3 << 5) ,
    IDTD_DEFAULT        = IDTD_PRESENT | INT_GATE_MASK, 
};

enum {
    DIV_BY_ZERO_INT = 0x00,
    SSI             = 0x01,
    NMI             = 0x02,
    OVERFLOW_INT    = 0x04,
    DOUBLE_F_INT    = 0x08,
    GP_INT          = 0x0D,
    PAGE_F_INT      = 0x0E,
};



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
    uint32_t error;
    uint32_t cs;
    uint32_t esp;
    uintptr_t eip;
};

void void_handler(struct isr_handler_args *);

static inline void cli(void) {
    __asm__ volatile ("cli");
}

typedef void (*isr_handler_t)(struct isr_handler_args *);

void add_irq_handler(uint8_t irq_num, isr_handler_t handler);
int add_isr_handler(uint8_t int_num, isr_handler_t handler, uint8_t flags);

int idt_init(void);

uint32_t disable_int(void);

void recover_int(uint32_t cflags);

static inline void sti(void) {
    __asm__ volatile ("sti");
}

static inline void halt(void) {
    __asm__ volatile ("hlt");
}

/* void isr_x86(struct x86_cpu_state, uint32_t int_num, struct isr_stack);  */
void isr_x86(uint32_t esp, struct isr_full_stack isr);

#endif
