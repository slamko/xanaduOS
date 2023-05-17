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

  char some[27];
  for (int i = 0; i < 26; i++) {
    some[i] = i + 'a';
  }
  some[26] = 0;

  fb_print_black(some);
  fb_print_num(123456789);
  /* shell_start(); */

  while (1)
    ;
}
