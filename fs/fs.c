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

struct dirent *readdir_fs(struct fs_node *node) {
    if (!node || !node->readdir) {
        return NULL;
    }

    return node->readdir(node);
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
