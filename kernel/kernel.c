#include "bin/shell.h"
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"
#include "drivers/keyboard.h"
#include "lib/slibc.h"
#include "mem/flat.h"
#include "mem/paging.h"
#include <stdint.h>

void kernel_main(void) {
  fb_clear();
  init_gdt();
  init_idt();

  /* shell_start(); */

  while (1);
}
