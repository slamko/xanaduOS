#include "drivers/int.h"
#include <stdint.h>
#include "kernel/error.h"
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

static int idt_set_entry(uint8_t idt_id, void *isr, uint8_t flags) {
    struct idt_entry *entry = &idt[idt_id];
    if (!entry) {
        return EINVAL;
    }
    
    entry->isr_low = (uint32_t)isr & 0xFFFF;
    entry->isr_high = (uint32_t)isr >> 16;
    entry->cs = 0x08;
    entry->attributes = flags;
    entry->reserved = 0;
    return 0;
}

void void_handler(struct isr_handler_args *args) {
    general_handler(args, "Unknown interrupt: ");
}

int idt_init(void) {
    idtr.base = (uint32_t)&idt;
    idtr.limit = sizeof(idt) - 1;

    for (uint8_t i = 0; i < 255; i++) {
        idt_set_entry(i, isr_table[i], IDTD_DEFAULT);
        isr_handlers[i] = &void_handler;
    }

    load_idt((uint32_t)&idtr);
    pic_init(0x00, false);
    exception_handlers_init();
    
    sti();

    klog("Interrupts enabled\n");
    return 0;
}

void add_irq_handler(uint8_t irq_num, isr_handler_t handler) {
    if (isr_handlers[PIC_REMAP + irq_num]) {
        klog_warn("Redefining irq handler\n");
        /* return; */
    }
    
    size_t int_num = PIC_REMAP + irq_num;
    isr_handlers[int_num] = handler;
    /* idt_set_entry(int_num, isr_table[int_num], IDTD_RING3 | IDTD_DEFAULT); */
    pic_unmask(irq_num);
}

int add_isr_handler(uint8_t int_num, isr_handler_t handler, uint8_t flags) {
    if (isr_handlers[int_num]) {
        klog_warn("Redefining isr handler\n");
        /* return; */
    }

    isr_handlers[int_num] = handler;

    if (flags) {
        return
            idt_set_entry(int_num, isr_table[int_num], flags | IDTD_DEFAULT);
    }
    return 0;
}

void isr_x86(uint32_t esp, struct isr_full_stack isr) {
    pic_eoi(isr.int_num);
    /* fb_newline(); */
    /* fb_print_num(isr.cs); */

    struct isr_handler_args args;
    args.int_id = isr.int_num;
    args.error = isr.error_code;
    args.eip = isr.eip;
    args.esp = esp;
    args.cs = isr.cs;
    args.cpu_regs = &isr.cpu_st;

    if (isr.cs > 0x1B) {
        /* klog("Cs :%x\n", isr.cpu_st.esp); */
    }

    if (isr.int_num == 33) {
        klog("Eip: %x\n", isr.cpu_st.eax);
    }
    
    count++;
    isr_handlers[isr.int_num](&args);

    /* fb_print_num(isr.eip); */

    if (isr.int_num == 0x80) {
        /* klog("Eip: %x\n", isr.eip); */
    }
 
}



