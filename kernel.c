#include <stdint.h>

char *const frame_buf = (char *)0xB8000;

enum VGA_COLORS {
    FB_BLACK = 0,
    FB_BLUE  = 1,
    FB_GREEN = 2,
    FB_CYAN  = 3,
    FB_GREY  = 8
};

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
   

void kernel_main(void) {
    char hello[] = "Hello, Kernel!";
    fb_print(hello, FB_CYAN, FB_GREY);

    while(1);
}
