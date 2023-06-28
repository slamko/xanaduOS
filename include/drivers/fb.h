#ifndef FB_H
#define FB_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include "drivers/dev.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_SIZE 80 * 25

enum VGA_COLORS {
    FB_BLACK         = 0,
    FB_BLUE          = 1,
    FB_GREEN         = 2,
    FB_CYAN          = 3,
    FB_RED           = 4,
    FB_MAGENTA       = 5,
    FB_BROWN         = 6,
    FB_LIGHT_GREY    = 7,
    FB_DARK_GREY     = 8,
    FB_LIGHT_BLUE    = 9,
    FB_LIGHT_GREEN   = 10,
    FB_LIGHT_CYAN    = 11,
    FB_LIGHT_RED     = 12,
    FB_LIGHT_MAGENTA = 13,
    FB_LIGHT_BROWN   = 14,
    FB_WHITE         = 15,
};

extern struct char_buffer fb_char_device;

struct fb_pixel {
    uint8_t symbol;
    uint8_t fg : 4;
    uint8_t bg : 4;
} __attribute__((packed));

struct fb_attr {
    uint8_t non_deletable: 1;
} __attribute__((packed));

extern char fb_out[VGA_SIZE];

void fb_flush(void);

void fb_print_char(uint16_t fb_index, uint8_t symbol,
                   uint8_t foreground, uint8_t background);

int fb_vprintf_color(const char *msg, uint8_t fg, uint8_t bg, va_list vargs);

void fb_delete_char(void);

void fb_delete_last(uint16_t len);

void fb_putc(uint8_t c);

void fb_last_written_buf(char **buf, size_t *len) ;

void fb_newline(void);

void fb_put_pixel(struct fb_pixel pixel);

void fb_print_pixels(struct fb_pixel *pixel, size_t len);

void fb_putc_attrs(uint8_t symbol, struct fb_attr attrs);

void fb_print_attrs(const char *msg, struct fb_attr attrs);

void fb_print(const char *msg, uint8_t fg, uint8_t bg);

void fb_print_black(const char *msg);

size_t fb_nprint(const char *msg, uint8_t fg, uint8_t bg, size_t siz);

size_t fb_nprint_black(const char *msg, size_t siz);

int fb_vprintf(const char *msg, va_list vargs);

int fb_printf(const char *msg, ...);

int fb_printf_color(const char *msg, unsigned int fg, unsigned int bg, ...);

void fb_print_num(unsigned int num);

void fb_print_hex(unsigned int num);

void fb_mov_cursor(uint16_t pos);

char *itoa(unsigned int num, char *buf, size_t siz,
           size_t *num_wr, int radix);

void fb_clear(void);
 

#endif
