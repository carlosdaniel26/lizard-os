#pragma once

#include <atomic.h>
#include <blkdev_manager.h>
#include <list.h>
#include <spinlock.h>
#include <types.h>

typedef struct Inode Inode;
typedef struct FsType FsType;

typedef struct Dentry {
    ListHead sibling;
    ListHead children;

    char name[NAME_MAX];
    u32 name_len;

    struct Dentry *parent;

    Inode *inode;

    spinlock_t lock;
    atomic_t refcount;

    u32 flags;
} Dentry;

typedef struct SuperBlock {
    ListHead list;
    const FsType *type;
    spinlock_t lock;

    void *fs_info;

    Dentry *root;
    u64 flags;

    size_t block_size;
    u64 total_blocks;
    u64 free_blocks;
} SuperBlock;

typedef struct {
    int (*statfs)(SuperBlock *sb, void *out);
    int (*sync)(SuperBlock *sb);
} SuperOps;

typedef struct FsType {
    ListHead list;
    char name[32];

    struct Dentry *(*mount)(SuperBlock *sb, const void *data);
    void (*kill_sb)(SuperBlock *sb);

    u32 flags;
    void *private_data;
} FsType;

typedef struct InodeOps {
    int (*lookup)(Inode *dir, Dentry *dentry);
    int (*create)(Inode *dir, Dentry *dentry, int mode);
    int (*mkdir)(Inode *dir, Dentry *dentry, int mode);
    int (*unlink)(Inode *dir, Dentry *dentry);
    int (*rename)(Inode *old, Inode *new);
} InodeOps;

typedef struct File {
    Inode *inode;
    u64 offset;
    u32 flags;
    void *private_data;
} File;

typedef struct FileOps {
    int (*open)(Inode *inode, File *file);
    ssize_t (*read)(File *file, char *buf, size_t count, off_t offset);
    ssize_t (*write)(File *file, const char *buf, size_t count, off_t offset);
    int (*readdir)(File *file, void *dirent, int (*filldir)(void *, const char *, int, off_t, u64));
    int (*release)(Inode *inode, File *file);
} FileOps;

typedef struct Inode {
    u32 mode;
    u64 size;

    InodeOps *i_ops;
    FileOps *f_ops;

    SuperBlock *sb;
    void *private_data;
} Inode;

int fs_register(FsType *type);
int fs_unregister(FsType *type);
FsType *fs_find(const char *name);
FsType *fs_find_locked(const char *name);
int fs_type_count(void);
FsType *fs_detect(BlockDevice *dev);

Dentry *dentry_alloc(const char *name);
void dentry_get(Dentry *d);
void dentry_put(Dentry *d);
Dentry *dentry_lookup(Dentry *parent, const char *name);
void dentry_add(Dentry *parent, Dentry *child);

Inode *inode_alloc(SuperBlock *sb);
void inode_free(Inode *inode);