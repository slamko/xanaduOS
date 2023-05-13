#include "lib/typedef.h"
#include <stdint.h>


struct idt_entry {
    uint16_t isr_low;
    uint16_t cs;
    uint8_t reserved;
    uint8_t attributes;
    uint16_t isr_high;
} PACKED;

struct idtr {
    uint16_t limit;
    uint32_t base;
} PACKED;

struct x86_cpu_state {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
} __attribute__((packed));

struct isr_stack {
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} __attribute__((packed));

#define CAT_(x,y) x##y
#define CAT(x,y) CAT_(x,y)

#define ISR(NUM) void CAT(isr_, NUM)(void);

void isr_x86(struct x86_cpu_state cpu_state,
             struct isr_stack int_stack, unsigned int int_num);

ISR(1);
ISR(2);

