#include <stdint.h>
#include "drivers/fb.h"

uint16_t row = 0;

void prompt() {
    fb_mov_cursor(row * 80);
    fb_print_black("hello>");
}

void loop(void) {
    fb_clear();

    for (int i = 0; i < 8; i++) {
        prompt();
        row ++; 
    }
}
    

