#pragma once

#include <atomic.h>
#include <list.h>
#include <spinlock.h>
#include <types.h>

typedef struct FsInstance FsInstance;
typedef struct Dentry Dentry;

typedef struct {
    int (*statfs)(SuperBlock *sb, void *out);
    int (*sync)(SuperBlock *sb);
} SuperOps;

typedef struct {
    ListHead list;
    char name[32];

    struct Dentry *(*mount)(SuperBlock *sb, const void *data);
    void (*kill_sb)(SuperBlock *sb);

    u32 flags;
    void *private_data;
} FsType;

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

typedef struct InodeOps {
    int (*lookup)(Inode *dir, Dentry *dentry);
    int (*create)(Inode *dir, Dentry *dentry, int mode);
    int (*mkdir)(Inode *dir, Dentry *dentry, int mode);
    int (*unlink)(Inode *dir, Dentry *dentry);
    int (*rename)(Inode *old, Inode *new);
} InodeOps;

typedef struct Inode {
    u32 mode;
    u64 size;

    InodeOps *i_ops;
    struct FileOps *f_ops;

    SuperBlock *sb;
    void *private_data;
} Inode;

int fs_register(FsType *type);
int fs_unregister(FsType *type);
FsType *fs_find(const char *name);
FsType *fs_find_locked(const char *name);
int fs_type_count(void);
