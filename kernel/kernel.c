#include "lib/kernel.h"
#include "bin/shell.h"
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "drivers/pit.h"
#include "drivers/serial.h"
#include "io.h"
#include "kernel/syscall.h"
#include "lib/slibc.h"
#include "mem/allocator.h"
#include "mem/flat.h"
#include "mem/paging.h"
#include "mem/slab_allocator.h"
#include "proc/proc.h"
#include "mem/buddy_alloc.h"
#include "drivers/rtc.h"
#include "drivers/floppy.h"
#include "drivers/storage/ata.h"
#include <stdint.h>

void jump_usermode(void);
void usermode_main(void);
void apic_init(void);

#define MB_MAGIC_NUM_DEF 0x1BADB002

#define MB_MEM 0
#define MB_BOOT_DEV 1
#define MB_CMDLINE 2
#define MB_MODS 3
#define MB_SYMS 4
#define MB_MMAP 6
#define MB_DRIVES 7
#define MB_CONFIG 8
#define MB_BOOTL_NAME 9
#define MB_APM 10
#define MB_VBE 11
#define MB_FB 12

#define MB_FLAGS_DEF ( \
    (1 << MB_MEM) \
    | (1 << MB_CMDLINE) \
)

#define MB_SECTION __attribute__((section(".multiboot.data")))
 
MB_SECTION unsigned int MB_MAGIC_NUM = MB_MAGIC_NUM_DEF;
MB_SECTION unsigned int MB_FLAGS = MB_FLAGS_DEF;
MB_SECTION unsigned int MB_CHECKSUM = -(MB_FLAGS_DEF + MB_MAGIC_NUM_DEF);
#define MB_FLAG(NUM) ((MB_FLAGS_DEF >> NUM) & 0xFFFFFFFF)

struct multiboot_meta {
    unsigned int flags;
#if MB_FLAG(MB_MEM)
    uintptr_t mem_lower;
    uintptr_t mem_upper;
#endif
#if MB_FLAG(MB_BOOT_DEV)
    unsigned int boot_dev;
#endif
#if MB_FLAG(MB_CMDLINE)
    unsigned int cmdline;
#endif
#if MB_FLAG(MB_MODS)
    unsigned int mods_count;
    unsigned int mods_addr;
#endif
#if MB_FLAG(MB_SYMS)
    unsigned int syms[3];
#endif
#if MB_FLAG(MB_MMAP)
    size_t mmap_len;
    uintptr_t mmap_addr;
#endif
#if MB_FLAG(MB_DRIVES)
    size_t drives_len;
    uintptr_t drives_addr;
#endif
#if MB_FLAG(MB_CONFIG)
    unsigned int config_table; 
#endif
#if MB_FLAG(MB_BOOTL_NAME)
    unsigned int bootloader_name;
#endif
#if MB_FLAG(MB_APM)
    unsigned int apm_table;
#endif
#if MB_FLAG(MB_VBE)
    unsigned int vbe_control_info;
    unsigned int vbe_mode_info;
    unsigned int vbe_mode;
    unsigned int vbe_interface_seg;
    unsigned int vbe_interface_off;
    unsigned int vbe_interface_len;
#endif
#if MB_FLAG(MB_FB)
    uintptr_t framebuffer_addr; 
    unsigned int framebuffer_pitch;
    unsigned int framebuffer_width;
    unsigned int framebuffer_heigh;
    unsigned int framebuffer_bpp;
    unsigned int framebuffer_type;
    unsigned int color_info;
#endif
} __attribute__((packed));

void print_multi_boot_data(struct multiboot_meta *mb) {
    fb_print_hex(mb->flags);
    fb_print_hex(mb->mem_lower);
    fb_print_hex(mb->mem_upper);
    fb_print_hex(mb->cmdline);
}

void kernel_main(struct multiboot_meta *multiboot_data) {
    /* struct multiboot_meta hm_mb_data; */
    /* memcpy(&hm_mb_data, multiboot_data, sizeof(hm_mb_data)); */
    
    fb_clear();

    gdt_init();
    idt_init();
    paging_init(multiboot_data->mem_upper * 0x400);

    serial_init();

    kbd_init();
    ps2_init();
    pit_init(0);
    rtc_init();

    syscall_init();
    /* floppy_init(); */

    /* sleep_sec(1); */
    ata_init();

    /* apic_init(); */

    /* jump_usermode(); */

    while (1) {
        /* reboot(); */
        
        cli();
        halt();
    }
}
