#include "drivers/initrd.h"
#include <stddef.h>
#include <stdint.h>
#include "drivers/int.h"
#include "lib/slibc.h"
#include "lib/kernel.h"
#include "mem/allocator.h"
#include "mem/buddy_alloc.h"
#include "mem/paging.h"
#include "mem/frame_allocator.h"
#include "fs/fs.h"
#include "mem/mmap.h"

struct tar_pax_header
{                              /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
};

#define HEADER_SIZE 512
#define HEADER_BLOCK_SIZE 1024

struct initrd_entry {
    struct tar_pax_header *header;
    uintptr_t data;
    struct initrd_entry *next;
};

struct initrd_entry *initrd;

unsigned int tar_parse(uintptr_t initrd_addr, size_t *size) {
    uintptr_t next_addr = initrd_addr;
    unsigned int i = 0;

    for (struct tar_pax_header *header = (struct tar_pax_header *)initrd_addr;
         header->name[0];
         header = (struct tar_pax_header *)next_addr) {

        if (i % 2) {
            struct initrd_entry *next = initrd ? initrd->next : NULL;
            initrd = kmalloc(sizeof(*initrd));
            initrd->next = next;
            initrd->header = header;
            initrd->data = next_addr + HEADER_SIZE;
        }

        size_t size = atoi(header->size, sizeof header->size, 8);
        next_addr += HEADER_SIZE + ((size / HEADER_SIZE) * HEADER_SIZE);
        if (size % HEADER_SIZE) {
            next_addr += HEADER_SIZE;
        }
        klog("Initrd file name: %s %x\n", header->name, next_addr - initrd_addr);
        
        i++;
    }

    *size = next_addr - initrd_addr;
    return i / 2;
}

int initrd_root_init() {

}

int initrd_init(struct module_struct *modules) {
    uintptr_t initrd_addr;
    size_t npages = (modules->mod_end - modules->mod_start) / PAGE_SIZE;

    int ret;
    ret = knmmap(cur_pd, &initrd_addr, modules->mod_start, npages, R_W | PRESENT);
    
    if (ret) {
        klog_error("Initrd was overwritten\n");
        return ret;
    }
   
    size_t initrd_size;
    unsigned int entry_num = tar_parse(initrd_addr, &initrd_size);
    klog("Initrd file name: %u\n", entry_num);

    return ret;
}
