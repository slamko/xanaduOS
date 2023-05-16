#ifndef FB_H
#define FB_H

#include <stddef.h>
#include <stdint.h>


enum VGA_COLORS {
    FB_BLACK = 0,
    FB_BLUE  = 1,
    FB_GREEN = 2,
    FB_CYAN  = 3,
    FB_GREY  = 8,
    FB_WHITE = 15

};

struct fb_attr {
    uint8_t non_deletable: 1;
} __attribute__((packed));
   
void fb_print_char(uint16_t fb_index, uint8_t symbol,
                   uint8_t foreground, uint8_t background);

void fb_putc(uint8_t c);

void fb_newline(void);

void fb_putc_attrs(uint8_t symbol, struct fb_attr attrs);

void fb_print_attrs(const char *msg, struct fb_attr attrs);

void fb_print(const char *msg, uint8_t fg, uint8_t bg);

void fb_print_black(const char *msg);

void fb_nprint(const char *msg, uint8_t fg, uint8_t bg, size_t siz);

void fb_nprint_black(const char *msg, size_t siz);

void fb_print_num(unsigned int num);

void fb_mov_cursor(uint16_t pos);

void fb_clear(void);
 

#endif
