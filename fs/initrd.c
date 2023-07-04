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
#include "mem/slab_allocator.h"
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

struct initrd_node {
    struct tar_pax_header *header;
    uintptr_t data;
    size_t size;
};

enum {
    FS_DIR = 0x1,
    FS_FILE = 0x0,
};

struct initrd_node *rd_nodes;
struct initrd_node *nodes;
struct fs_node *rd_fs;

struct slab_cache *fs_cache;
struct slab_cache *initrd_cache;

size_t initrd_read(struct fs_node *node, uint32_t offset, size_t size, uint8_t *buf) {
    unsigned int fd = i / 2;
    initrd_list[fd].header = header;
    initrd_list[fd].data = next_addr + HEADER_SIZE;

    rd_fs[fd].size = file_size;
    rd_fs[fd].close = NULL;
    rd_fs[fd].open = NULL;
    rd_fs[fd].readdir = NULL;
    rd_fs[fd].write = NULL;
    rd_fs[fd].finddir = NULL;
    rd_fs[fd].read = &initrd_read;
}

unsigned int tar_parse(struct initrd_entry **rd_list,
                       uintptr_t initrd_addr, size_t *size) {
    uintptr_t next_addr = initrd_addr;
    char *dir_prefix;
    unsigned int i = 0;

    for (struct tar_pax_header *header = (struct tar_pax_header *)initrd_addr;
         header->name[0];
         header = (struct tar_pax_header *)next_addr) {

        size_t file_size = atoi(header->size, sizeof header->size, 8);

        if (i % 2) {
            struct initrd_entry *next = *rd_list ? (*rd_list)->next : NULL;
            *rd_list = slab_alloc_from_cache(initrd_cache);

            struct initrd_entry *initrd_list = *rd_list;
            initrd_list->next = next;
            initrd_list->data = to_uintptr(header) + HEADER_SIZE;
            initrd_list->header = header;
        }

        next_addr += align_up(HEADER_SIZE + file_size, HEADER_SIZE);

        klog("Initrd file name: %s %x\n",
             header->name, next_addr - initrd_addr);
        
        i++;
    }

    *size = (next_addr - initrd_addr);
    return i / 2;
}

int initrd_build_fs(size_t nodes_n) {
    for (unsigned int i = 0; i < nodes_n; i++) {
        rd_fs[i].size = rd_nodes[i].header->size;
        rd_fs[i].close = NULL;
        rd_fs[i].open = NULL;
        rd_fs[i].readdir = NULL;
        rd_fs[i].write = NULL;
        rd_fs[i].finddir = NULL;
        rd_fs[i].read = &initrd_read;
    }
}

int initrd_build_tree(struct initrd_entry *initrd_list, size_t rd_len) {
    unsigned int i = rd_len;

    foreach(ent, initrd_list,
            rd_nodes[i].data = ent->data;
            rd_nodes[i].header = ent->header;

            if (i == 1) {
                break;
            }

            i--;
        );
    
    return i;
}

int initrd_init(struct module_struct *modules, struct fs_node *root) {
    uintptr_t initrd_addr;
    size_t npages = (modules->mod_end - modules->mod_start) / PAGE_SIZE;
    struct initrd_entry *rd_list;

    int ret;
    ret = knmmap(cur_pd, &initrd_addr, modules->mod_start, npages, R_W | PRESENT);
    
    if (ret) {
        klog_error("Initrd was overwritten\n");
        return ret;
    }

    fs_cache = slab_cache_create(sizeof(struct fs_node));
    initrd_cache = slab_cache_create(sizeof(struct initrd_entry));
   
    size_t initrd_len =
        (modules->mod_end - modules->mod_start) / HEADER_BLOCK_SIZE;

    unsigned int entry_num = tar_parse(&rd_list, initrd_addr, &initrd_len);
    klog("Initrd file name: %u\n", entry_num);

    rd_nodes = kmalloc(entry_num * sizeof(*rd_nodes));
    rd_fs = kmalloc(entry_num * sizeof(*rd_fs));

    initrd_build_tree(rd_list, entry_num);
    slab_cache_destroy(initrd_cache);
    
    initrd_build_fs();

    return ret;
}
