#include "drivers/keyboard.h"
#include "drivers/fb.h"
#include "drivers/int.h"
#include "drivers/mouse.h"
#include "lib/kernel.h"
#include "lib/slibc.h"
#include <stddef.h>
#include <stdint.h>

receiver receiver_f[KBD_INT_REC_NUM];
unsigned int receiver_num;

uint32_t kbd_buf;

static size_t read_pos;
static size_t buf_pos;

static uint32_t kbd_US[128] = {
    0,          27,   '1', '2',  '3',  '4',  '5',  '6', '7',
    '8',        '9',  '0', '-',  '=',  '\b', '\t', /* <-- Tab */
    'q',        'w',  'e', 'r',  't',  'y',  'u',  'i', 'o',
    'p',        '[',  ']', '\n', 0, /* <-- control key */
    'a',        's',  'd', 'f',  'g',  'h',  'j',  'k', 'l',
    ';',        '\'', '`', 0,    '\\', 'z',  'x',  'c', 'v',
    'b',        'n',  'm', ',',  '.',  '/',  0,    '*', 0, /* Alt */
    ' ',                                                   /* Space bar */
    0,                                                     /* Caps lock */
    0, /* 59 - F1 key ... > */
    0,          0,    0,   0,    0,    0,    0,    0,   0, /* < ... F10 */
    0,                                                     /* 69 - Num lock*/
    0,                                                     /* Scroll Lock */
    0,                                                     /* Home key */
    UP_ARROW,                                              /* Up Arrow */
    0,                                                     /* Page Up */
    '-',        0,                                         /* Left Arrow */
    0,          0,                                         /* Right Arrow */
    '+',        0,                                         /* 79 - End key*/
    DOWN_ARROW,                                            /* Down Arrow */
    0,                                                     /* Page Down */
    0,                                                     /* Insert Key */
    KBD_DEL,                                               /* Delete Key */
    0,          0,    0,   0,                              /* F11 Key */
    0,                                                     /* F12 Key */
    0, /* All other keys are undefined */
};

uint32_t read_scan_code() {
    uint32_t c = ps2_read_data();
    return kbd_US[c];
}

void kbd_add_receiver(receiver f) {
    receiver_f[receiver_num] = f;
    receiver_num++;
}

void kbd_interrupt(struct isr_handler_args args) {
    uint8_t stat = ps2_read_status();

    if (stat & (1 << 0)) {
        uint32_t keycode = read_scan_code();

        if (keycode && keycode < KBD_LAYOUT_MAX_CODE) {
            kbd_buf = keycode;

            for (size_t r_id = 0; receiver_f[r_id]; r_id++) {
                receiver_f[r_id](keycode);
            }
        }
    }
}

uint32_t kbd_read(void) { return kbd_buf; }

void kbd_init(void) {
    add_irq_handler(KBD_IRQ, &kbd_interrupt);
    klog("Keyboard interrupts actived\n");
}
