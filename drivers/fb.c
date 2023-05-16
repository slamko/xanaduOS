#include "drivers/fb.h"
#include "lib/typedef.h"
#include "drivers/keyboard.h"
#include "lib/slibc.h"
#include <stddef.h>
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
static struct fb_attr frame_buf_attrs[VGA_SIZE];

void scroll() {    
    /* memcpy(frame_copy_buf, frame_buf, VGA_SIZE); */
    /* fb_clear(); */
    
    for (uint16_t i = 80; i < VGA_SIZE; i++) {
        frame_buf[i - 80] = frame_buf[i];
    }

    for (uint16_t i = VGA_SIZE - 80; i < VGA_SIZE; i++) {
        frame_buf[i].symbol = 0;
    }
    
    fb_pos -= 80;
}

void fb_putc_attrs(uint8_t symbol, struct fb_attr attrs) {
    frame_buf_attrs[fb_pos] = attrs;
    fb_putc(symbol);
}

void fb_print_attrs(const char *msg, struct fb_attr attrs) {
    for (uint16_t i = 0; msg[i] != 0; i++) {
        fb_putc_attrs(msg[i], attrs);
    }
}

void fb_print_char(uint16_t offset, uint8_t symbol,
                   uint8_t foreground, uint8_t background) {
    if (!symbol) return;

    uint16_t fb_i = fb_pos + offset;
    if (symbol == '\n') {
        fb_newline();
        return;
    }

    if (symbol == '\t') {
        fb_print_black("    ");
        return;
    }

    if (symbol == KBD_DEL) {
        if (frame_buf_attrs[fb_i - 1].non_deletable) {
            return;
        }
        
        fb_pos = fb_i - 1;
        frame_buf[fb_pos].symbol = 0;

        fb_mov_cursor(fb_pos);
        return;
    }

    if (fb_i >= VGA_SIZE) {
        scroll();
        fb_i = fb_pos + offset;
    }

    frame_buf[fb_i].symbol = symbol;
    frame_buf[fb_i].bg = background;
    frame_buf[fb_i].fg = foreground;
   
    fb_pos += offset + 1;

    if (!frame_buf[fb_pos].symbol) {
        frame_buf[fb_pos].symbol = 0;
        frame_buf[fb_pos].bg = background;
        frame_buf[fb_pos].fg = foreground;
    }
    
    fb_mov_cursor(fb_pos);
}

void fb_putc(uint8_t c) {
    fb_print_char(0, c, FB_WHITE, FB_BLACK);
}

void fb_print(const char *msg, uint8_t fg, uint8_t bg) {
    for (uint16_t i = 0; msg[i] != 0; i++) {
        fb_print_char(0, msg[i], fg, bg);
    }
}

void fb_nprint(const char *msg, uint8_t fg, uint8_t bg, size_t siz) {
    for (uint16_t i = 0; i < siz && msg[i] != 0; i++) {
        fb_print_char(0, msg[i], fg, bg);
    }
}

void fb_print_black(const char *msg) {
    fb_print(msg, FB_WHITE, FB_BLACK);
}


void fb_nprint_black(const char *msg, size_t siz) {
    fb_nprint(msg, FB_WHITE, FB_BLACK, siz);
}

void fb_newline(void) {
    fb_column += 1;

    if (fb_column > VGA_HEIGHT) {
        fb_column = 0;
    }

    fb_pos = (fb_pos - (fb_pos % 80)) + 80;
}

char *_print_num_rec(unsigned int num, uint32_t mul, char *str, size_t siz) {
    if (num >= 10) {
        uint32_t div = (uint32_t)(num / 10);
        char c = num - (div * 10) + 48;
        str[siz - mul - 1] = c;
        return _print_num_rec(div, mul + 1, str, siz);
    } else {
        char c = num + 48;
        str[siz - mul - 1] = c;
        return str + siz - mul - 1;
    }

    return str;
}

void fb_print_num(unsigned int num) {
    /* char str[16] = {0}; */
    /* char *str_num = _print_num_rec(num, 1, str, sizeof(str)); */
    /* fb_print_black(str_num); */
    if (num >= 1000) {
        fb_putc((uint8_t)(num / 1000) + 48);
    } else if (num >= 100) {
        fb_putc((uint8_t)(num / 100) + 48);
        fb_putc((uint8_t)((num / 10) % 10) + 48);
        fb_putc((uint8_t)(num - ((num / 10) * 10)) + 48);
    } else if (num >= 10) {
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


    
