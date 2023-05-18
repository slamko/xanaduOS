#include "bin/shell.h"
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"
#include "drivers/keyboard.h"
#include "lib/slibc.h"
#include "mem/flat.h"
#include "mem/paging.h"
#include <stdint.h>

void some(void);

void test() {

  char *some = malloc(27);
  char *sec = malloc(27);
  for (int i = 0; i < 26; i++) {
    some[i] = i + 'a';
  }
  /* fb_print_black(some); */
    fb_print_num((uint32_t)&some);
  for (int i = 0; i < 26; i++) {
    sec[i] = i + '0';
  }
    fb_print_num((uint32_t)&sec);
  some[26] = 0;
  sec[26] = 0;
  /* fb_print_black(sec); */
  free(some);
  free(sec);


}

void kernel_main(void) {
  fb_clear();
  init_gdt();
  init_idt();
  /* some(); */
  /* asm volatile ("int $0x6"); */
  shell_start();

  while (1)
    ;
}
