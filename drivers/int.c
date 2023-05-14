#include "drivers/int.h"
#include "drivers/fb.h"
#include "drivers/shell.h"
#include "lib/typedef.h"
#include "lib/slibc.h"
#include <stdint.h>

static struct idt_entry idt[256];
static struct idtr idtr;
static int count;

extern void *isr_table[];

void idt_set_entry(uint8_t idt_id, void *isr, uint8_t flags) {
    struct idt_entry *entry = &idt[idt_id];
    entry->isr_low = (uint32_t)isr & 0xFFFF;
    entry->isr_high = (uint32_t)isr >> 16;
    entry->cs = 0x08;
    entry->attributes = flags;
    entry->reserved = 0;
}

static char kbd_US [128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', /* <-- Tab */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     
    0, /* <-- control key */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    99,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};
/* reinitialize the PIC controllers, giving them specified vector offsets
   rather than 8h and 70h, as configured by default */
 
#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */
 
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

char buf[256];

/*
arguments:
	offset1 - vector offset for master PIC
		vectors on the master become offset1..offset1+7
	offset2 - same for slave PIC: offset2..offset2+7
*/
void pic_remap(int offset1, int offset2)
{
	uint8_t a1, a2;
 
	a1 = inb(PIC1_DATA);                        // save masks
	a2 = inb(PIC2_DATA);
 
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
// starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);
// ICW2: Master PIC vector offset

    io_wait();
	outb(PIC2_DATA, offset2);
// ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4);
// ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2);
// ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
 
	outb(PIC1_DATA, ICW4_8086);
// ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();
 
	outb(PIC1_DATA, a1);   // restore saved masks.
	outb(PIC2_DATA, a2);
}

void init_idt() {
    idtr.base = (uint32_t)&idt;
    idtr.limit = sizeof(idt) - 1;

    for(uint8_t i = 0; i < 32; i++) {
        idt_set_entry(i, isr_table[i], 0x8e);
    }

    asm volatile ("lidt %0" : : "m" (idtr));
    outb(PIC1_DATA, 0xfd);
    asm volatile ("sti");

    /* pic_remap(PIC1, PIC2); */
}

void pic_eoi(uint8_t int_id) {
    if (int_id >= PIC2) {
        outb(PIC2_COMMAND, PIC_EOI);
    } else if (int_id >= PIC1) {
        outb(PIC1_COMMAND, PIC_EOI);
    }
}

uint8_t read_scan_code() {
    uint8_t c = inb(KBD_INPUT_PORT);
    return kbd_US[c];
    /* return c; */
}

void isr_x86(struct x86_cpu_state cpu_state,
             struct isr_stack int_stack, unsigned int int_num) {
    (void)cpu_state;
    (void)int_stack;
    
    count++;
    pic_eoi(PIC1);
    uint8_t a;
    if ((a = inb(0x64))) {
        uint8_t keycode = read_scan_code();
        fb_putc(keycode);
        buf[strlen(buf)] = keycode;

        if (keycode == '\n') {
            read_buf(buf);
        }

        /* fb_newline(); */

        /* read_scan_code(); */
        /* fb_putc('f'); */
    }
    
    asm volatile ("cli;");
}



