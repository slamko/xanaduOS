#include "lib/kernel.h"
#include "bin/shell.h"
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "drivers/pit.h"
#include "drivers/serial.h"
#include "fs/fs.h"
#include "io.h"
#include "ipc/pci.h"
#include "kernel/syscall.h"
#include "lib/slibc.h"
#include "mem/allocator.h"
#include "mem/paging.h"
#include "mem/slab_allocator.h"
#include "proc/proc.h"
#include "mem/buddy_alloc.h"
#include "drivers/rtc.h"
#include "drivers/floppy.h"
#include "drivers/storage/ata.h"
#include "net/ethernet/rtl8139.h"
#include "drivers/initrd.h"
#include "mem/mmap.h"
#include "kernel/error.h"
#include <stddef.h>
#include <stdint.h>
#include <elf.h>

void usermode_main(void);
void apic_init(void);
void lookup_pci_dev(void);

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
    | (1 << MB_BOOT_DEV) \
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
/* #if MB_FLAG(MB_MODS) */
    unsigned int mods_count;
    unsigned int mods_addr;
/* #endif */
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
    /* fb_print_hex(mb->cmdline); */
}

void mem_test(void) {
    struct new {
        int a;
        struct new *next;
        uintptr_t some;
        struct x86_cpu_state st;
    };

    struct simple {
        struct simple *next;
        uintptr_t a;
    };

    struct some {
        struct new next;
        int a;
        char *one;
        char y;
        struct x86_cpu_state *st;
    };
/*
    for (unsigned int i = 0; i < 1; i++) {
        struct slab_cache **cache = kmalloc(sizeof(*cache) * 0x100);
        void ***addrs = kmalloc(sizeof(*addrs) * 0x100);
        struct slab_cache *addr_cache = slab_cache_create(sizeof (**addrs) * 0x1000);

        for (size_t i = 0; i < 256; i++) {
            cache[i] = slab_cache_create(sizeof **cache);

            addrs[i] = slab_alloc_from_cache(addr_cache);
            for (size_t j = 0; j < 0x100; j++) {
                addrs[i][j] = slab_alloc_from_cache(cache[i]);
            }
        }

        for (size_t i = 0; i < 0x100; i++) {

            for (size_t j = 0; j < 0x100; j++) {
                slab_free(cache[i], addrs[i][j * 8]);
            }
        }

        kfree(addrs);
        slab_cache_destroy(addr_cache);

        for (size_t i = 0; i < 0x100; i++) {

            slab_cache_destroy(cache[i]);
        }
    }
*/

    kmalloc(0xe00000);
    struct slab_cache *cache = slab_cache_create_align(PAGE_SIZE, PAGE_SIZE);
    for (size_t i = 0; i < 0x100; i++) {
        char *c = slab_alloc_from_cache(cache);
        kmalloc(0x100);
        /* klog("Addr %x\n", c); */
        c[0x50] = 'a';
    }

    kmalloc(0x300000);
    for (size_t i = 0; i < 0x100; i++) {
        char *c = slab_alloc_from_cache(cache);
        kmalloc(0x100);
        /* klog("Addr %x\n", c); */
        c[0x50] = 'a';
    }
}

void kernel_main(struct multiboot_meta *multiboot_data) {
    struct multiboot_meta hm_mb_data;
    memcpy(&hm_mb_data, multiboot_data, sizeof(hm_mb_data));
    
    struct module_struct s;
    memcpy(&s, (void *)(hm_mb_data.mods_addr), sizeof(s));

    fb_clear();
    gdt_init();
    idt_init();

    paging_init(multiboot_data->mem_upper * 0x400);
    kmmap_init();
    serial_init();

    kbd_init();
    ps2_init();
    pci_init();
    
    pit_init(0);
    apic_init();
    rtc_init();
    multiproc_init();

    /* mem_test(); */
    klog("Hello %x\n", to_phys_addr(cur_pd, map_limit - 1));

    syscall_init();

    /* buddy_test(0); */
    /* slab_test(); */

    spawn_init(&s);
    /* klog("Kernel gain\n"); */

    while (1) {
        /* reboot(); */
        
        /* cli(); */
        /* halt(); */
    }
}
