#include "lib/typedef.h"
#include "lib/slibc.h"
#include "drivers/keyboard.h"

receiver receiver_f[KBD_INT_REC_NUM];
char kbd_buf[1024];
static size_t buf_pos;

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
    127,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

uint8_t read_scan_code() {
    uint8_t c = inb(KBD_INPUT_PORT);
    return kbd_US[c];
}

void interrupt() {
    uint8_t stat = inb(KBD_STATUS_PORT);
    if (stat) {
        uint8_t keycode = read_scan_code();

        if (keycode) {
            kbd_buf[buf_pos] = keycode;
            buf_pos++;
        }
        
        for (size_t r_id = 0; receiver_f[r_id]; r_id++) {
            receiver_f[r_id](keycode);
        }

        if (keycode == '\n') {
            memset(kbd_buf, 0, sizeof(kbd_buf));
            buf_pos = 0;
        }
    }
 
}
