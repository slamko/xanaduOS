#include "drivers/fb.h"
#include "lib/kernel.h"
#include "drivers/keyboard.h"
#include "lib/slibc.h"
#include "mem/paging.h"
#include "drivers/dev.h"
#include <bits/types.h>
#include <stddef.h>
#include <stdint.h>

#define FB_COMMAND_PORT         0x3D4
#define FB_DATA_PORT            0x3D5

/* The I/O port commands */
#define FB_HIGH_BYTE_COMMAND    14
#define FB_LOW_BYTE_COMMAND     15

static uint16_t fb_pos;

/* static struct fb_pixel *frame_buf = (struct fb_pixel *)0xC03FF000; */
static struct fb_pixel *const frame_buf = (struct fb_pixel *)0xb8000;
static struct fb_attr frame_buf_attrs[VGA_SIZE];
char fb_out[VGA_SIZE];

struct char_buffer fb_char_device = {
    .buf = fb_out,
    .pos = 0,
    .size = VGA_SIZE,
    .flush = &fb_flush,
};

static inline void write_fb_sync(uint16_t i, struct fb_pixel pixel) {
    frame_buf[i] = pixel;
}

static inline void write_fb_local(uint16_t i, struct fb_pixel pixel) {
    frame_buf[i] = pixel;
    fb_out[i] = pixel.symbol;
}

void (*volatile write_fb)(uint16_t i, struct fb_pixel pixel) = &write_fb_local;

static inline void set_fb_pos(uint16_t new_pos) {
    fb_pos = new_pos;
    fb_char_device.pos = new_pos;
}

static void scroll(void) {    
    for (uint16_t i = 80; i < VGA_SIZE; i++) {
        write_fb(i - 80, frame_buf[i]);
        frame_buf_attrs[i - 80] = frame_buf_attrs[i];
    }

    for (uint16_t i = VGA_SIZE - 80; i < VGA_SIZE; i++) {
        write_fb(i, (struct fb_pixel) {0});
        frame_buf_attrs[i] = (struct fb_attr) {0};
    }
    
    set_fb_pos(fb_pos - 80);
}

void fb_flush(void) {
    /* fb_print_black("hello"); */
    write_fb = &write_fb_sync;
    for (size_t i = fb_pos; i <= fb_char_device.pos; i++) {
        fb_putc(fb_out[i]);
    }
    write_fb = &write_fb_local;
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

    if (symbol == '\r') return;

    struct fb_pixel pixel = {
        .symbol = symbol,
        .fg = foreground,
        .bg = background
    };

    uint16_t fb_i = fb_pos + offset;
    if (symbol == '\n') {
        /* fb_out[fb_i] = symbol; */
        fb_newline();
        return;
    }

    if (symbol == '\t') {
        fb_print_black("    ");
        return;
    }

    if (symbol == KBD_DEL) {
        fb_delete_char();
        return;
    }

    if (fb_i >= VGA_SIZE) {
        scroll();
        fb_i = fb_pos + offset;
    }

    write_fb(fb_pos, pixel);
    set_fb_pos(offset + 1 + fb_pos);

    if (!frame_buf[fb_pos].symbol) {
        pixel.symbol = 0;
        write_fb(fb_pos, pixel);
    }
    
    fb_mov_cursor(fb_pos);
}

void fb_delete_char(void) {
    if (frame_buf_attrs[fb_pos - 1].non_deletable) {
        return;
    }

    set_fb_pos(fb_pos - 1);
    struct fb_pixel del_pix = frame_buf[fb_pos];
    del_pix.symbol = 0;
    write_fb(fb_pos, del_pix);

    fb_mov_cursor(fb_pos);
 
}

void fb_delete_last(uint16_t len) {
    for (int i = 0; (i++) < len; fb_delete_char());
}

void fb_last_written_buf(char **buf, size_t *len) {
    size_t i;
    for (i = fb_pos; !frame_buf_attrs[i].non_deletable; i--);

    *len = fb_pos - i - 1;
    *buf = &fb_out[i + 1];
}

void fb_putc(uint8_t c) {
    fb_print_char(0, c, FB_WHITE, FB_BLACK);
}

void fb_print(const char *msg, uint8_t fg, uint8_t bg) {
    for (uint16_t i = 0; msg[i] != 0; i++) {
        fb_print_char(0, msg[i], fg, bg);
    }
}

size_t fb_nprint(const char *msg, uint8_t fg, uint8_t bg, size_t siz) {
    size_t i;
    for (i = 0; i < siz && msg[i] != 0; i++) {
        fb_print_char(0, msg[i], fg, bg);
    }
    return i;
}

void fb_put_pixel(struct fb_pixel pixel) {
    fb_print_char(0, pixel.symbol, FB_WHITE, pixel.bg);
}

void fb_print_pixels(struct fb_pixel *pixel, size_t len) {
    for (uint16_t i = 0; i < len; i++) {
        fb_put_pixel(pixel[i]);
    }
}

void fb_print_black(const char *msg) {
    fb_print(msg, FB_WHITE, FB_BLACK);
}

size_t fb_nprint_black(const char *msg, size_t siz) {
    return fb_nprint(msg, FB_WHITE, FB_BLACK, siz);
}

void fb_newline(void) {
    set_fb_pos((fb_pos - (fb_pos % 80)) + 80);

    if (fb_pos >= VGA_SIZE) {
        scroll();
    }
}

char *_print_num_rec(unsigned int num, uint32_t *mul, char *str, size_t siz) {
    if (num >= 10) {
        uint32_t div = (uint32_t)(num / 10);
        char c = num - (div * 10) + 48;
        str[siz - *mul - 1] = c;
        *mul += 1;
        return _print_num_rec(div, mul, str, siz);
    } else {
        char c = num + 48;
        str[siz - *mul - 1] = c;
        return str + siz - *mul - 1;
    }

    return str;
}

void fb_print_num(unsigned int num) {
    char str[16];
    uint32_t mul = 1;
    char *str_num = _print_num_rec(num, &mul, str, sizeof(str));
    fb_nprint_black(str_num, mul);
    fb_newline();
}

char dec_to_hex(unsigned int num) {
    if (num >= 10) {
        return num + 48 + 7;
    }
    return num + 48;
}

char *_print_hex_rec(unsigned int num, uint32_t *mul, char *str, size_t siz) {
    if (num >= 16) {
        uint32_t div = (uint32_t)(num / 16);
        /* fb_print_num(div); */
        char c = dec_to_hex(num - (div * 16)); 
        str[siz - *mul - 1] = c;
        *mul += 1;
        return _print_hex_rec(div, mul, str, siz);
    } else {
        char c = dec_to_hex(num);
        str[siz - *mul - 1] = c;
        return str + siz - *mul - 1;
    }

    return str;
}

void fb_print_hex(unsigned int num) {
    char str[16];
    uint32_t mul = 1;
    char *str_num = _print_hex_rec(num, &mul, str, sizeof(str));
    fb_nprint_black(str_num, mul);
    fb_newline();
}

void fb_mov_cursor(uint16_t pos) {
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    outb(FB_DATA_PORT, (pos >> 8) & 0xFF);
    outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    outb(FB_DATA_PORT, pos & 0xFF);
}

void fb_clear(void) {
    for (size_t i = 0; i < VGA_SIZE; i++) {
        frame_buf[i] = (struct fb_pixel) {0};
        frame_buf_attrs[i] = (struct fb_attr) {0};
        fb_out[i] = 0;
    }
    fb_pos = 0;
}   


    
