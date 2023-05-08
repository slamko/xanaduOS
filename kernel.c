typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

#define FB_BLACK 0
#define FB_BLUE  1
#define FB_GREEN 2
#define FB_CYAN  3
#define FB_GREY  8

void fb_print_char(u16 fb_index, u8 symbol, u8 foreground, u8 background) {

    char *const frame_buf = (char *)0xB8000;
    frame_buf[fb_index] = symbol;
    frame_buf[fb_index + 1] = ((background << 4) | (foreground & 0x0f));
}

void fb_print(const char *msg, u16 size, u8 fg, u8 bg) {
    for (u16 i = 0; i < size; i++) {
        fb_print_char(i * 2, msg[i], fg, bg);
    }
}
   

void kernel_main(void) {
    char hello[] = "Hello";
    fb_print(hello, 5, FB_CYAN, FB_GREY);

    while(1);
}
