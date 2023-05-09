#include <stdint.h>
#include "drivers/fb.h"

void kernel_main(void) {
    fb_mov_cursor(0);
    fb_clear();
    
    fb_mov_cursor(0);
    char hello[] = "Hello, Kernel!";
    fb_print(hello, FB_CYAN, FB_GREY);

    while(1);
}
