#include <stdint.h>
#include "drivers/fb.h"
#include "drivers/shell.h"
#include "lib/slibc.h"

void shell_prompt() {
    /* fb_newline(); */
    fb_print_black("slavos> ");
}

uint8_t execute(const char *buf) {
    return 0;
}

void read_buf(const char *buf) {
    execute(buf);
    /* fb_print_black(buf); */

    shell_prompt();
}

void shell(void) {
    shell_prompt();
    while (1) {
    }
}
    

