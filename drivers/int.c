#include "int.h"

static struct idt_entry idt[256];

static void *isr_table[] = {
    &isr_1,
    &isr_2
};

void isr_x86(struct x86_cpu_state cpu_state,
             struct isr_stack int_stack, unsigned int int_num) {

}



