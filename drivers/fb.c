#include "drivers/fb.h"
#include "lib/typedef.h"
#include "lib/slibc.h"
#include <stdint.h>

#define FB_COMMAND_PORT         0x3D4
#define FB_DATA_PORT            0x3D5

/* The I/O port commands */
#define FB_HIGH_BYTE_COMMAND    14
#define FB_LOW_BYTE_COMMAND     15

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_SIZE 80 * 25

static uint16_t fb_column;
static uint16_t fb_pos;

struct fb_pixel {
    uint8_t symbol;
    uint8_t fg : 4;
    uint8_t bg : 4;
} __attribute__((packed));

static struct fb_pixel *const frame_buf = (struct fb_pixel *)0xB8000;

void fb_print_char(uint16_t fb_index, uint8_t symbol,
                   uint8_t foreground, uint8_t background) {

    /* uint16_t fb_i = (fb_column * VGA_WIDTH) + fb_index; */
    uint16_t fb_i = fb_pos + fb_index;
    if (fb_i > VGA_SIZE) {
        fb_i -= VGA_SIZE;
    }

    frame_buf[fb_i].symbol = symbol;
    frame_buf[fb_i].bg = background;
    frame_buf[fb_i].fg = foreground;
    fb_pos++;
}

void fb_putc(uint8_t c) {
    fb_print_char(0, c, FB_WHITE, FB_BLACK);
}

void fb_print(const char *msg, uint8_t fg, uint8_t bg) {
    for (uint16_t i = 0; msg[i] != 0; i++) {
        fb_print_char(i, msg[i], fg, bg);
    }
}

void fb_print_black(const char *msg) {
    fb_print(msg, FB_WHITE, FB_BLACK);
}

void fb_newline(void) {
    fb_column += 1;

    if (fb_column > VGA_HEIGHT) {
        fb_column = 0;
    }

    fb_pos = (fb_pos - (fb_pos % 80)) + 80;
}

void fb_print_num(unsigned int num) {
    if (num > 1000) {
        fb_print_black("big");
    } else if (num > 100) {
        fb_putc((uint8_t)(num / 100) + 48);
        fb_putc((uint8_t)(num / 10) + 48);
        fb_putc((uint8_t)(num % 10) + 48);
    
    } else if (num > 10) {
        fb_putc((uint8_t)(num / 10) + 48);
        fb_putc((uint8_t)(num % 10) + 48);
    } else {
        fb_putc(num + 48);
    }
}

void fb_mov_cursor(uint16_t pos) {
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    outb(FB_DATA_PORT, (pos >> 8) & 0xFF);
    outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    outb(FB_DATA_PORT, pos & 0xFF);
}

void fb_clear(void) {
    memset(frame_buf, 0, VGA_SIZE);
}   


    
