#include "fb.h"
#include <stdint.h>

#define FB_COMMAND_PORT         0x3D4
#define FB_DATA_PORT            0x3D5

/* The I/O port commands */
#define FB_HIGH_BYTE_COMMAND    14
#define FB_LOW_BYTE_COMMAND     15

static volatile char *const frame_buf = (char *)0xB8000;

void fb_print_char(uint16_t fb_index, uint8_t symbol,
                   uint8_t foreground, uint8_t background) {

    frame_buf[fb_index] = symbol;
    frame_buf[fb_index + 1] = ((background << 4) | (foreground & 0x0f));
}

void fb_print(const char *msg, uint8_t fg, uint8_t bg) {
    for (uint16_t i = 0; msg[i] != 0; i++) {
        fb_print_char(i * 2, msg[i], fg, bg);
    }
}

void fb_mov_cursor(uint16_t pos) {
    outfb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    outfb(FB_DATA_PORT, 0x0);
    outfb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    outfb(FB_DATA_PORT, 0x0);
}

void fb_clear(void) {
    for (uint16_t i = 0; i < 1024; i++) {
        frame_buf[i] = 0;
    }
}   


    
