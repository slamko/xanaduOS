#include <stddef.h>
#include <stdint.h>
#include "drivers/fb.h"
#include "bin/shell.h"
#include "lib/slibc.h"
#include "drivers/keyboard.h"
#include "mem/flat.h"

void shell_prompt() {
    fb_print_attrs("slavos> ", (struct fb_attr) {.non_deletable = 1});
}

void shell_start(void) {
    fb_clear();
    receiver_f[0] = &read_stream;
    shell_prompt();
}

int execute(const char *cmd, size_t len) {
    if (strneq(cmd, "echo", 4)) {
        fb_print_black(cmd + 5);
        return 0;
    } else if (cmd[0] != '\n' || len == 0) {
        fb_print_black("Unknown command");
        fb_newline();
    }
    return 1;
}

int read_stream(unsigned char c) {
    fb_putc(c);

    if (c == '\n') {
        /* fb_print_black(k_buf); */
        /* fb_newline(); */
        char *cmd = NULL;
        size_t len;
        fb_last_written_buf(&cmd, &len);
        execute(cmd, len);
        shell_prompt();
    }

    return 0;
}

