#include "bin/shell.h"
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "lib/slibc.h"
#include "mem/flat.h"
#include "drivers/serial.h"
#include "io.h"
#include <stdint.h>

void jump_usermode(void);

void kernel_main(void) {
  fb_clear();

  init_gdt();
  init_idt();
  serial_init();
  kbd_init();
  ps2_init();
  
  /* while (1) { */
      /* wait_serial_read(COM1); */
        /* serial_send(COM1, 'a'); */
  /* } */
  /* serial_write(COM1, "Hello, World", 13); */

  /* serial_shell(); */
  /* fprintf(fb_char_device, "Hello world\n"); */
  /* flush(fb_char_device); */
  /* fb_print_num(123456); */
  /* asm volatile ("int $0x6"); */
  /* asm volatile ("int $0x7"); */
  /* asm volatile("int $128"); */
  /* asm volatile ("int $0x80"); */
  /* jump_usermode(); */

  /* fb_newline(); */
  /* shell_start(); */
  /* fb_print_black("rello"); */

  while (1);
}
