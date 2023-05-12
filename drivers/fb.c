#include "fb.h"
#include "lib/typedef.h"
#include "lib/slibc.h"

#define FB_COMMAND_PORT         0x3D4
#define FB_DATA_PORT            0x3D5

/* The I/O port commands */
#define FB_HIGH_BYTE_COMMAND    14
#define FB_LOW_BYTE_COMMAND     15

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_SIZE 80 * 25

static uint16_t fb_column;

struct fb_pixel {
    uint8_t symbol;
    uint8_t fg : 4;
    uint8_t bg : 4;
} __attribute__((packed));

static struct fb_pixel *const frame_buf = (struct fb_pixel *)0xB8000;

void fb_print_char(uint16_t fb_index, uint8_t symbol,
                   uint8_t foreground, uint8_t background) {

    uint16_t fb_i = (fb_column * VGA_WIDTH) + fb_index;
    if (fb_i > VGA_SIZE) {
        fb_i -= VGA_SIZE;
    }

    frame_buf[fb_i].symbol = symbol;
    frame_buf[fb_i].bg = background;
    frame_buf[fb_i].fg = foreground;
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
}

/* void outfb(uint16_t port, uint8_t data) { */
    /* int a; */
    /* asm volatile */
        /* ("movl %[port], %al;" */
        /* "movl %[data], %dx;" */
        /* "out %al, %dx;" */
         /* : "=r" (a) */
         /* : [port] "r" (port) */
         /* : [data] "r" (data)); */
/* } */
void outb(uint16_t port, uint16_t value) {
    asm volatile ("out %1, %0" : : "dN" (port), "a" (value));
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


    
