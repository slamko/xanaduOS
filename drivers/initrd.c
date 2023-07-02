#include <stdint.h>
#include "lib/slibc.h"
#include "drivers/initrd.h"
#include "lib/kernel.h"
#include "initrd.h"

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

struct module_struct {
    uintptr_t mod_start;
    uintptr_t mod_end;
    char *string;
    int reserved;
} __attribute__((packed));

void initrd_init(struct module_struct *mod_struct) {
    struct tar_pax_header tar;

    if (!mod_struct) {
        /* klog_warn("No Initrd found\n"); */
    }
    
    memcpy(&tar, &header, sizeof(tar)); 
    
    int siz = atoi(tar.size, sizeof tar.size, 8);
    klog("Initrd file name: %u\n", siz);
}
