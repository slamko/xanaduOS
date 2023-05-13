#include "drivers/int.h"
#include <stdint.h>

static struct idt_entry idt[256];
static struct idtr idtr;

static void *isr_table[] = {
    &isr_1,
    &isr_2
};

void idt_set_entry(uint8_t idt_id, void *isr, uint8_t flags) {
    struct idt_entry *entry = &idt[idt_id];
    entry->isr_low = (uint32_t)isr & 0xFFFF;
    entry->isr_high = (uint32_t)isr >> 16;
    entry->cs = 0x08;
    entry->attributes = flags;
    entry->reserved = 0;
}

void init_idt() {
    idtr.base = (uint32_t)&idt;
    idtr.limit = sizeof(idt) - 1;

    for(uint8_t i = 0; i < 2; i++) {
        idt_set_entry(i, isr_table[i], 0x8e);
    }

    asm volatile ("lidt %0;"
                  "sti;" : : "m" (idtr));
    
}

void isr_x86(struct x86_cpu_state cpu_state,
             struct isr_stack int_stack, unsigned int int_num) {
    asm volatile ("cli;");
}



