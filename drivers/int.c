#include "drivers/int.h"
#include <stdint.h>

static struct idt_entry idt[256];

static void *isr_table[] = {
    &isr_1,
    &isr_2
};

void idt_set_entry(uint8_t idt_id, void *isr, uint8_t flags) {
    struct idt_entry *entry = &idt[idt_id];
    entry->isr_low = (uint32_t)isr & 0xFFFF;
    entry->isr_high = (uint32_t)isr >> 16;
    entry->attributes = flags;
    entry->reserved = 0;
}

void isr_x86(struct x86_cpu_state cpu_state,
             struct isr_stack int_stack, unsigned int int_num) {

}



