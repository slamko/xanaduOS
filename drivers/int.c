#include <stdint.h>

#include "drivers/int.h"
#include "drivers/fb.h"
#include "drivers/pic.h"
#include "drivers/keyboard.h"
#include "lib/typedef.h"
#include "lib/slibc.h"

static struct idt_entry idt[256];
static struct idtr idtr;
static int count;

extern void *isr_table[];

static void idt_set_entry(uint8_t idt_id, void *isr, uint8_t flags) {
    struct idt_entry *entry = &idt[idt_id];
    entry->isr_low = (uint32_t)isr & 0xFFFF;
    entry->isr_high = (uint32_t)isr >> 16;
    entry->cs = 0x08;
    entry->attributes = flags;
    entry->reserved = 0;
}

void isr_128(void);

void syscall_int(void) {
    fb_print_black("syscall");
}

void load_idt(uint32_t ptr);

void cli(void) {
    asm volatile ("cli");
}

static inline void sti(void) {
    asm volatile ("sti");
}

void init_idt() {
    idtr.base = (uint32_t)&idt;
    idtr.limit = sizeof(idt) - 1;

    for (uint8_t i = 0; i < 32; i++) {
        idt_set_entry(i, isr_table[i], 0x8e);
    }

    /* for (uint8_t i = 80; i < ARR_SIZE(idt) - 1; i++) { */
        /* isr_table[i] = &isr_128; */
    /* } */

    isr_table[SYSCALL - 1] = isr_128;
    isr_table[SYSCALL] = isr_128;
    idt_set_entry(SYSCALL - 1, isr_table[SYSCALL - 1], 0x8e);
    idt_set_entry(SYSCALL, isr_table[SYSCALL], 0x8e);

    load_idt((uint32_t)&idtr);

    pic_init(KBD_MASK);
    sti();

    fb_print_black("Interrupts enabled\n");
}

/* void isr_x86(struct x86_cpu_state c, uint32_t int_num, */
             /* struct isr_stack stack) {  */
void isr_x86(struct isr_full_stack isr) {
    
    count++;
    pic_eoi(PIC1);
    interrupt();

    fb_newline();
    fb_print_num(isr.int_num);
    /* fb_newline(); */
    /* fb_print_num(pic_get_irr()); */
    /* fb_newline(); */
    /* fb_print_num(pic_get_isr()); */
    /* fb_newline(); */
    /* fb_print_num(cpu_state.ebp); */
    /* fb_newline(); */
    /* fb_print_num(int_stack.error_code); */

   
    cli();
}



