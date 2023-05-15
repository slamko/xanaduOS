#include <stddef.h>
#include <stdint.h>
#include "drivers/fb.h"
#include "bin/shell.h"
#include "lib/slibc.h"

static char buf[256];
static size_t buf_pos;

void shell_prompt() {
    /* fb_newline(); */
    fb_print_black("slavos> ");
}

void shell_start(void) {
    shell_prompt();
}

int execute() {
    char echo[] = "echo";
    if (strncmp(buf, echo, 4) == 0) {
        fb_print_black(buf + 4);
        return 0;
    } 
    fb_print_black("Unknown command");
    fb_newline();
    return 1;
}

int read_stream(unsigned char c) {
    fb_putc(c);
    buf[buf_pos] = c;
    buf_pos++;

    if (c == '\n') {
        execute();
        buf_pos = 0;
        memset(buf, 0, sizeof(buf));
        shell_prompt();
    }

    return 0;
}

