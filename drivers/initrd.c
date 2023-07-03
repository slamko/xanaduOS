#include "drivers/initrd.h"
#include <stdint.h>
#include "drivers/int.h"
#include "lib/slibc.h"
#include "lib/kernel.h"
#include "mem/buddy_alloc.h"
#include "mem/paging.h"
#include "mem/frame_allocator.h"
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

int initrd_init(struct module_struct *modules) {
    struct tar_pax_header tar;
    uintptr_t initrd_addr;

    int ret;
    ret = kmmap(cur_pd, &initrd_addr, modules->mod_start);
    
    if (ret) {
        klog_error("InitRD was overwritten\n");
        return ret;
    }
   
    memcpy(&tar, (void *)initrd_addr, sizeof(tar));
    
    int siz = atoi(tar.size, sizeof tar.size, 8);
    klog("Initrd file name: %s\n", tar.name);
    return ret;
}
