/*
  #include "kernel/syscall.h"

#include <stddef.h>
#include <stdint.h>

static char last_cmd[1024];
static char cur_cmd[1024];

void shell_prompt() {
    syscall(1, "slavos> ", 8);
}

void shell_start(void) {
    shell_prompt();
}

int execute(const char *cmd, size_t len) {
    syscall(1, "Unknown command\n", 16);
    return 1;
}

int read_stream(uint32_t c) {
    char *cmd = NULL;
    size_t len;

    if (c == '\n') {
         fb_print_black(k_buf); 
         fb_newline(); 

        execute(cmd, len);
        shell_prompt();
    }


    return 0;
}

void usershell() {

}


*/
