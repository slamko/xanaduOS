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

struct initrd_node {
    struct tar_pax_header *header;
    uintptr_t data;
    size_t size;
    size_t sub_ent_num;

    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t type;

};

struct initrd_entry {
    struct initrd_node node;
    struct initrd_entry *next;
};

enum {
    FS_DIR = 0x1,
    FS_FILE = 0x0,
};

struct initrd_node *rd_nodes;
struct fs_node *rd_fs;

struct slab_cache *fs_cache;
struct slab_cache *dir_cache;
struct slab_cache *initrd_cache;

size_t initrd_read(struct fs_node *node, uint32_t offset, size_t size, uint8_t *buf) {
}

struct DIR *initrd_opendir(struct fs_node *node) {
    struct initrd_node *ent = &rd_nodes[node->inode];
    size_t allocation = ent->sub_ent_num * sizeof(struct dirent);
    struct DIR *dir = kmalloc(sizeof(*dir) + allocation);

    /* dir->data = (struct dirent *)(void *)(to_uintptr(dir) + sizeof(*dir)); */
    dir->node = node;
    dir->ofset = 0;

    return dir;
}

struct dirent *initrd_readdir(struct DIR *dir) {
    inode_t ent_inode = dir->node->inode + dir->ofset + 1;
    struct initrd_node *ent = &rd_nodes[ent_inode];
    struct initrd_node *dir_node = &rd_nodes[dir->node->inode];

    if (dir->ofset >= dir_node->sub_ent_num) {
        return NULL;
    }

    struct dirent *dirent = (struct dirent *)&dir->data[dir->ofset];

    strcpy(dirent->name, ent->header->name, sizeof dirent->name);
    dirent->inode = ent_inode;

    dir->ofset++;
    return dirent;
}

void initrd_closedir(struct DIR *dir) {
    kfree(dir);
}

struct fs_node *initrd_get_root(void) {
    return &rd_fs[0];
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
        uint32_t gid = atoi(header->gid, sizeof header->gid, 8);
        uint32_t uid = atoi(header->uid, sizeof header->uid, 8);
        uint32_t mode = atoi(header->mode, sizeof header->mode, 8);

        if (i % 2) {
            struct initrd_entry *next = *rd_list;
            *rd_list = slab_alloc_from_cache(initrd_cache);

            struct initrd_entry *initrd_list = *rd_list;
            initrd_list->next = next;

            initrd_list->node.size = file_size;
            initrd_list->node.gid = gid;
            initrd_list->node.uid = uid;
            initrd_list->node.mode = mode;
            initrd_list->node.type = FS_FILE;

            initrd_list->node.data = to_uintptr(header) + HEADER_SIZE;
            initrd_list->node.header = header;
            /* klog("Initrd file name: %s %x\n", */
                /* header->name, next_addr - initrd_addr); */
        }

        next_addr += align_up(HEADER_SIZE + file_size, HEADER_SIZE);

        
        i++;
    }

    *size = (next_addr - initrd_addr);
    return i / 2;
}

int initrd_build_fs(size_t nodes_n) {
    for (unsigned int i = 0; i < nodes_n; i++) {
        struct fs_node *node = &rd_fs[i];

        strcpy(node->name, rd_nodes[i].header->name,
               sizeof(rd_nodes[i].header->name));

        node->gid = rd_nodes[i].gid;
        node->uid = rd_nodes[i].uid;
        node->type = rd_nodes[i].type;
        node->mode = rd_nodes[i].mode;
        node->size = rd_nodes[i].size;

        node->close = NULL;
        node->open = NULL;
        node->write = NULL;
        node->finddir = NULL;
        node->readdir = NULL;

        if (node->type == FS_DIR) {
            node->readdir = &initrd_readdir;
            node->closedir = &initrd_closedir;
            node->opendir = &initrd_opendir;
        }

        node->read = &initrd_read;
        node->this = node;
    }

    return 0;
}

int initrd_build_tree(struct initrd_entry *initrd_list, size_t rd_len) {
    unsigned int i = rd_len;

    rd_nodes = kmalloc((i + 1) * sizeof(*rd_nodes));
    /* klog("Create root node %x\n", rd_nodes); */

    struct initrd_entry *ent = initrd_list;
    foreach(ent, 
            memcpy(&rd_nodes[i], &ent->node, sizeof(ent->node));
            /* klog("Initrd node name %s\n", ent->node.header->name); */

            if (i == 1) {
                break;
            }

            i--;
        );

    struct initrd_node *root = &rd_nodes[0];
    root->type = FS_DIR;
    root->header = kmalloc(sizeof(*root->header));
    root->sub_ent_num = 2;
    strcpy(root->header->name, "/", 1);
    
    return rd_len;
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

    /* dir_cache = slab_cache_create(sizeof(struct dirent)); */
    initrd_cache = slab_cache_create(sizeof(struct initrd_entry));
   
    size_t initrd_len =
        (modules->mod_end - modules->mod_start) / HEADER_BLOCK_SIZE;

    unsigned int entry_num = tar_parse(&rd_list, initrd_addr, &initrd_len);

    rd_fs = kmalloc(entry_num * sizeof(*rd_fs));

    size_t node_num = initrd_build_tree(rd_list, entry_num);
    /* klog("Parsed files num %u:\n", align_up(14, 3)); */
    slab_cache_destroy(initrd_cache);
    

    initrd_build_fs(node_num);

    return ret;
}

