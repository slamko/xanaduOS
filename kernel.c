#include <stdint.h>
#include "drivers/fb.h"

uint16_t row = 0;

void prompt() {
    fb_print_black("slavos>");
    fb_newline();
}

void loop(void) {
    fb_clear();

    for (int i = 0; i < 8; i++) {
        prompt();
        row ++; 
    }
}

void kernel_main(void) {
    fb_clear();

    prompt();
    prompt();

    while(1);
}
