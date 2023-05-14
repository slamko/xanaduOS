#include <stdint.h>
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"

uint16_t row = 0;

void prompt() {
    fb_print_black("slavos>");
    fb_newline();
}

void shell(void) {
    fb_clear();

    for (int i = 0; i < 8; i++) {
        prompt();
        row ++; 
    }
}

void kernel_main(void) {
    init_gdt();
    init_idt();
    fb_clear();
    
    /* shell(); */
    asm volatile("int $0x02");
    asm volatile("int $0x04");
    
    while(1);
}
