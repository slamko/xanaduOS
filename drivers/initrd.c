#include "drivers/initrd.h"
#include <stdint.h>
#include "drivers/int.h"
#include "lib/slibc.h"
#include "lib/kernel.h"
#include "mem/buddy_alloc.h"
#include "mem/paging.h"

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

    uintptr_t tar_paddr;

    int ret;
    page_table_t pt;
    uint16_t pde, pte;
    get_pde_pte(modules->mod_start, &pde, &pte);

    ret = map_alloc_pt(cur_pd, &pt, pde);
    if (ret) {
        return 1;
    }

    klog("Initrd file name: %x\n", pt); 
    ret = buddy_alloc_at_addr(modules->mod_start, &pt[pte], 1, R_W | PRESENT);
    if (ret) {
        return 1;
    }

    /* flush_page(get_ident_phys_page_addr(pde, pte)); */

    
    memcpy(&tar, (void *)(modules->mod_start), sizeof(tar));
    
    /* int siz = atoi(tar.size, sizeof tar.size, 8); */
    /* for (unsigned int i = 0; i < sizeof(val); i++) { */
        /* if (val[i] >= 'a' && val[i] <= 'z') { */
    klog("Initrd file name: %s\n", tar.name);
        /* } */
    /* } */
}
