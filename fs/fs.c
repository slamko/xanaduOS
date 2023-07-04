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

struct fs_node *finddir_fs(struct fs_node *node, char *name) {
    if (!node || !node->finddir) {
        return NULL;
    }

    return node->finddir(node, name);
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

size_t mmap_fs(struct fs_node *node, uintptr_t *addrs, size_t size,
               uint16_t flags) {
    if (!node || !node->mmap) {
        return 0;
    }

    return node->mmap(node, addrs, size, flags);
}


