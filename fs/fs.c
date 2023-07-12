#include "fs/fs.h"
#include "kernel/error.h"
#include <stddef.h>
#include <stdint.h>

struct fs_node *fs_root;

int read_fs(struct fs_node *node, uint32_t offset, size_t len, uint8_t *buf) {
    if (!node || !node->read) {
        return EINVAL;
    }

    return node->read(node, offset, len, buf);
}

int write_fs(struct fs_node *node, uint32_t offset, size_t len, uint8_t *buf) {
    if (!node || !node->write) {
        return EINVAL;
    }

    return node->write(node, offset, len, buf);
}

struct DIR *opendir_fs(struct fs_node *node) {
    if (!node || !node->opendir) {
        return NULL;
    }

    return node->opendir(node);
}

void closedir_fs(struct DIR *dir) {
    if (!dir || !dir->node->closedir) {
        return;
    }
    
    return dir->node->closedir(dir);
}

struct dirent *readdir_fs(struct DIR *dir) {
    if (!dir || !dir->node || !dir->node->readdir) {
        return NULL;
    }

    return dir->node->readdir(dir);
}

void open_fs(struct fs_node *node) {
    if (!node || !node->open) {
        return;
    }

    node->open(node);
}

void close_fs(struct fs_node *node) {
    if (!node || !node->readdir) {
        return;
    }

    node->close(node);
}

struct fs_node *root_get_node_fs(const char *name) {
    if (!name) {
        return NULL;
    }

    return fs_root->get_node(fs_root, name);
}

size_t mmap_fs(struct fs_node *node, uintptr_t *addrs, size_t npages,
               uint16_t flags) {
    if (!node || !node->mmap) {
        return 0;
    }

    return node->mmap(node, addrs, npages, flags);
}

void munmap_fs(struct fs_node *node, uintptr_t addr) {
    if (!node || !node->munmap) {
        return;
    }

    return node->munmap(node, addr);
}

