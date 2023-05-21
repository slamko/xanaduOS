#include "bin/shell.h"
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"
#include "drivers/keyboard.h"
#include "lib/slibc.h"
#include "mem/flat.h"
#include "drivers/serial.h"
#include "io.h"
#include <stdint.h>

void kernel_main(void) {
  fb_clear();

  init_gdt();
  init_idt();
  serial_init();

  /* fb_print_black("hello"); */
  serial_send(COM1, 'a');

  /* serial_shell(); */
  /* fprintf(fb_char_device, "Hello world\n"); */
  /* flush(fb_char_device); */
  /* fb_print_num(123456); */
  /* asm volatile ("int $0x6"); */
  asm volatile("int $128");
  /* shell_start(); */

  while (1)
    ;
}
