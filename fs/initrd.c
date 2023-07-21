#include "drivers/initrd.h"
#include <stddef.h>
#include <stdint.h>
#include "drivers/int.h"
#include "kernel/error.h"
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
    FS_FILE = 0x0,
    FS_DIR = 0x1,
};

struct initrd_node *rd_nodes;
struct fs_node *rd_fs;

struct slab_cache *fs_cache;
struct slab_cache *dir_cache;
struct slab_cache *initrd_cache;

size_t initrd_read(struct fs_node *node, uint32_t offset,
                   size_t size, uint8_t *buf) {
    if (!buf) {
        return 0;
    }
    
    void *read_addr = (void *)(rd_nodes[node->inode].data + offset);
    memcpy(buf, read_addr, size);
    return node->size;
}

void print_node(struct fs_node *node) {
    klog("Node name: %s\n", node->name);
    klog("Node inode: %u\n", node->inode);
    klog("Node sub entry num: %u\n", rd_nodes[node->inode].sub_ent_num);
    klog("Node type: %d\n", node->type);
}

struct DIR *initrd_opendir(struct fs_node *node) {
    struct initrd_node *ent = &rd_nodes[node->inode];
    size_t allocation = ent->sub_ent_num * sizeof(struct dirent);
    struct DIR *dir = kmalloc(sizeof(*dir) + allocation);

    klog("Root sub entry num %d\n", rd_nodes[0].sub_ent_num);
    dir->node = node;
    dir->ofset = 0;

    return dir;
}

void initrd_munmap(struct fs_node *node, uintptr_t addr) {
    (void)node;
    (void)addr;
}

size_t initrd_mmap(struct fs_node *node, uintptr_t *addrs,
                   size_t size, uint16_t flags) {
    struct initrd_node *rd_node = &rd_nodes[node->inode];

    uintptr_t start_paddr = page_align_down(
        ptr_to_phys_addr(cur_pd, rd_node->header));

    klog("Virt et phys map addr %x\n", *addrs);
    set_addrs(addrs, start_paddr, size, flags);

    return rd_node->data - page_align_down(to_uintptr(rd_node->header));
}

struct dirent *initrd_readdir(struct DIR *dir) {
    struct initrd_node *dir_node = &rd_nodes[dir->node->inode];
    if (dir->ofset >= dir_node->sub_ent_num) {
        return NULL;
    }

    inode_t ent_inode = dir->node->inode + dir->ofset + 1;
    struct initrd_node *ent = &rd_nodes[ent_inode];

    /* print_node(&rd_fs[ent_inode]); */
    /* print_node(&rd_fs[dir->node->inode]); */
    struct dirent *dirent = &dir->data[dir->ofset];
    strcpy(dirent->name, ent->header->name, sizeof(ent->header->name));
    dirent->node = &rd_fs[ent_inode];

    dir->ofset++;
    return dirent;
}

void print_header(int inode) {
    if (rd_nodes && rd_nodes[inode].header) {
        klog("SZieof %u\n", check_block_size(rd_nodes));
        klog("Print header initrd %x\n", rd_nodes[inode].header);
    }
}

void initrd_closedir(struct DIR *dir) {
    /* inode_t ent_inode = dir->node->inode + dir->ofset; */
    /* struct initrd_node *ent = &rd_nodes[ent_inode]; */
    kfree(dir);
    /* klog("Dir offset close %x\n", ent->header); */
}

struct fs_node *initrd_get_root(void) {
    return &rd_fs[0];
}

struct fs_node *initrd_get_node(struct fs_node *root, const char *name) {
    struct fs_node *node;
    struct DIR *root_dir = opendir_fs(root);

    for (struct dirent *ent = readdir_fs(root_dir);
         ent;
         ent = readdir_fs(root_dir)) {

        if (strcmp(ent->name, name) == 0) {
            klog("Execve filename: %s\n", ent->name);
            node = ent->node;
        }
    }

    closedir_fs(root_dir);
    return node;
}

static const char *parse_file_name(const char *fname, size_t *len) {
    size_t offset = *len - 1;

    while (offset > 0) {
        offset--;
        
        if (fname[offset] == '/') {
            offset++;
            break;
        }
    }

    if (fname[*len - 1] == '/') {
        *len -= 1;
    }

    return fname + offset;
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

            if (header->name[strnlen(header->name, sizeof header->name) - 1]
                 == '/') {
                initrd_list->node.type = FS_DIR;
            } else {
                initrd_list->node.type = FS_FILE;
            }

            initrd_list->node.data = to_uintptr(header) + HEADER_SIZE;
            initrd_list->node.header = header;
            /* klog("Initrd header location %s\n", header->name); */
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

        const char *node_name = rd_nodes[i].header->name;
        size_t fname_len = strnlen(node_name, sizeof(rd_nodes[i].header->name));
        const char *fname = parse_file_name(node_name, &fname_len);
        ptrdiff_t offset = fname - node_name;
        /* klog("offset: %d ; fname_len %d \n", offset, fname_len - offset); */

        strcpy(node->name, fname, sizeof(rd_nodes[i].header->name));
        node->name[fname_len - offset] = 0;
        klog("Registered node name %s\n", node->name);

        node->gid = rd_nodes[i].gid;
        node->uid = rd_nodes[i].uid;
        node->type = rd_nodes[i].type;
        node->mode = rd_nodes[i].mode;
        node->size = rd_nodes[i].size;

        node->close = NULL;
        node->open = NULL;
        node->write = NULL;
        node->readdir = NULL;
        node->mmap = &initrd_mmap;
        node->munmap = &initrd_munmap;
        node->inode = i;

        if (node->type == FS_DIR) {
            node->readdir = &initrd_readdir;
            node->get_node = &initrd_get_node;
            node->closedir = &initrd_closedir;
            node->opendir = &initrd_opendir;
        }

        /* klog("FS Build node %x\n", rd_nodes[i].header); */
        node->read = &initrd_read;
        node->this = node;
    }

    return 0;
}


int initrd_build_tree(struct initrd_entry *initrd_list, size_t rd_len) {
    unsigned int i = rd_len;

    rd_nodes = kmalloc((i + 1) * sizeof(*rd_nodes));

    struct initrd_entry *ent = initrd_list;
    foreach(ent, 
            memcpy(&rd_nodes[i], &ent->node, sizeof(ent->node));

            if (i == 1) {
                break;
            }

            i--;
        );

    struct initrd_node *root = &rd_nodes[0];
    root->type = FS_DIR;
    root->header = kmalloc(sizeof(*root->header));
    root->sub_ent_num = rd_len;
    strcpy(root->header->name, "/", 1);
    
    return rd_len + 1;
}

int initrd_init(struct module_struct *modules, struct fs_node *root) {
    uintptr_t *initrd_addr;
    size_t mod_size = modules->mod_end - modules->mod_start;
    size_t npages = mod_size / PAGE_SIZE;
    if (mod_size % PAGE_SIZE) {
        npages++;
    }
    
    struct initrd_entry *rd_list;

    int ret;
    initrd_addr = kmalloc(npages * sizeof(*initrd_addr));
    klog("Initrd map page number %d\n", npages);

    ret = knmmap(kern_buddy, cur_pd, initrd_addr, modules->mod_start, npages, R_W | PRESENT);
    
    if (ret) {
        klog_error("Initrd was overwritten\n");
        return ret;
    }

    /* dir_cache = slab_cache_create(sizeof(struct dirent)); */
    initrd_cache = slab_cache_create(sizeof(struct initrd_entry));
   
    size_t initrd_len =
        (modules->mod_end - modules->mod_start) / HEADER_BLOCK_SIZE;

    unsigned int entry_num = tar_parse(&rd_list, *initrd_addr, &initrd_len);

    size_t node_num = initrd_build_tree(rd_list, entry_num);
    /* slab_cache_destroy(initrd_cache); */
    
    rd_fs = kmalloc(node_num * sizeof(*rd_fs));
    initrd_build_fs(node_num);

    return ret;
}

