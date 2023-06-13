#include <stdint.h>

#include "drivers/int.h"
#include "drivers/fb.h"
#include "drivers/serial.h"
#include "kernel/syscall.h"
#include "drivers/pic.h"
#include "drivers/keyboard.h"
#include "int/except_handler.h"
#include "lib/kernel.h"
#include "lib/slibc.h"
#include "drivers/gdt.h"

static struct idt_entry idt[256] __attribute__((aligned(4)));
static struct idtr idtr;
static int count;

extern void *isr_table[];

static isr_handler_t isr_handlers[256];

void load_idt(uint32_t ptr);

static void idt_set_entry(uint8_t idt_id, void *isr, uint8_t flags) {
    struct idt_entry *entry = &idt[idt_id];
    entry->isr_low = (uint32_t)isr & 0xFFFF;
    entry->isr_high = (uint32_t)isr >> 16;
    entry->cs = 0x08;
    entry->attributes = flags;
    entry->reserved = 0;
}

static void void_handler(struct isr_handler_args args) {
    fb_newline();
    fb_print_black("Unknown interrupt");
    fb_print_num(args.int_id);
    fb_newline();
}



void init_idt() {
    idtr.base = (uint32_t)&idt;
    idtr.limit = sizeof(idt) - 1;

    for (uint8_t i = 0; i < 255; i++) {
        idt_set_entry(i, isr_table[i], IDTD_DEFAULT);
        isr_handlers[i] = &void_handler;
    }

    load_idt((uint32_t)&idtr);
    pic_init(0, true);
    exception_handlers_init();
    
    sti();

    klog("Interrupts enabled\n");
}

void add_irq_handler(uint8_t irq_num, isr_handler_t handler) {
    isr_handlers[PIC_REMAP + irq_num] = handler;
    pic_unmask(irq_num);
}

void add_isr_handler(uint8_t int_num, isr_handler_t handler, uint8_t flags) {
    isr_handlers[int_num] = handler;

    if (flags) {
        idt_set_entry(int_num, isr_table[int_num], flags);
    }
}

void isr_x86(struct isr_full_stack isr) {
    /* pic_eoi(isr.int_num); */
    /* fb_newline(); */
    /* fb_print_num(isr.cs); */
    
    count++;
    isr_handlers[isr.int_num](
        (struct isr_handler_args) {
            .int_id = isr.int_num,
            .eip = isr.eip
        });

    /* fb_print_num(isr.int_num); */
}



