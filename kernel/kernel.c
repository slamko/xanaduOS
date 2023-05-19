#include "bin/shell.h"
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"
#include "drivers/keyboard.h"
#include "lib/slibc.h"
#include "mem/flat.h"
#include "drivers/serial.h"
#include <stdint.h>

void kernel_main(void) {
  fb_clear();

  init_gdt();
  init_idt();
  serial_init();

  serial_send(COM1, '\r');
  serial_send(COM1, '\n');
  serial_send(COM1, 'a');
  serial_send(COM1, '\r');
  serial_send(COM1, '\n');
  serial_send(COM1, 'b');
  serial_send(COM1, 127);
  serial_send(COM1, 127);
  
  /* fb_print_num(123456); */
  /* asm volatile ("int $0x6"); */
  shell_start();

  while (1)
    ;
}
