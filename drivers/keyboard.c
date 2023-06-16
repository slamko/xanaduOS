#include "drivers/fb.h"
#include "drivers/int.h"
#include "lib/kernel.h"
#include "lib/slibc.h"
#include "drivers/keyboard.h"
#include <stddef.h>
#include <stdint.h>

receiver receiver_f[KBD_INT_REC_NUM];
unsigned int receiver_num;

uint32_t kbd_buf[1024];

static size_t read_pos;
static size_t buf_pos;

static uint32_t kbd_US [128] =
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
    UP_ARROW,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    DOWN_ARROW,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    KBD_DEL,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

uint32_t read_scan_code() {
    uint32_t c = inb(KBD_INPUT_PORT);
    return kbd_US[c];
}

void kbd_add_receiver(receiver f) {
    receiver_f[receiver_num] = f;
}

void kbd_interrupt(struct isr_handler_args args) {
    uint8_t stat = inb(KBD_STATUS_PORT);

    if (stat & (1 << 0)) {
        uint32_t keycode = read_scan_code();

        if (keycode && keycode < KBD_LAYOUT_MAX_CODE) {
            kbd_buf[buf_pos] = keycode;
            buf_pos++;

            for (size_t r_id = 0; receiver_f[r_id]; r_id++) {
                receiver_f[r_id](keycode);
            }

            if (keycode == '\n') {
                memset(kbd_buf, 0, sizeof(kbd_buf));
                buf_pos = 0;
            }
        }
    }
}

uint32_t kbd_read(void) {
    if (read_pos == buf_pos) return 0;
    
    read_pos ++;
    return kbd_buf[read_pos - 1];
}

void kbd_init(void) {
    add_irq_handler(KBD_IRQ, &kbd_interrupt);
}
