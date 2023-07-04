#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

typedef size_t inode_t;
struct fs_node;
struct dirent;

extern struct fs_node *fs_root;

typedef size_t (*fs_read_f)(struct fs_node *, uint32_t, size_t size, uint8_t * buf);
typedef size_t (*fs_write_f)(struct fs_node *, uint32_t, size_t size, uint8_t * buf);

typedef void (*fs_open_f)(struct fs_node *);
typedef void (*fs_close_f)(struct fs_node *);

struct DIR {
    unsigned int ofset;
    struct fs_node *node;
    char data[] __attribute__((aligned(__alignof(long double))));
    /* struct dirent *data; */
};

typedef struct DIR *(*fs_opendir_f)(struct fs_node *);
typedef void (*fs_closedir_f)(struct DIR *);

typedef struct dirent *(*fs_readdir_f)(struct DIR*);
typedef struct fs_node *(*fs_finddir_f)(struct fs_node *, char *name);

struct fs_node {
    char name[256];
    inode_t inode;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t type;
    size_t size;

    fs_read_f read;
    fs_write_f write;
    fs_open_f open;
    fs_close_f close;

    fs_readdir_f readdir;
    fs_finddir_f finddir;
    fs_opendir_f opendir;
    fs_closedir_f closedir;

    struct fs_node *this;
};

struct dirent {
    char name[256];
    inode_t inode;
};

int read_fs(struct fs_node *node, uint32_t offset, size_t len, uint8_t *buf);

int write_fs(struct fs_node *node, uint32_t offset, size_t len, uint8_t *buf);

struct DIR *opendir_fs(struct fs_node *node);

void closedir_fs(struct DIR *dir);

struct dirent *readdir_fs(struct DIR *dir);

struct fs_node *finddir_fs(struct fs_node *node, char *name);

void open_fs(struct fs_node *node);

void close_fs(struct fs_node *node);

#endif
