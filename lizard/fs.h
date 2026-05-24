#pragma once

#include <lizard/atomic.h>
#include <lizard/blkdev_manager.h>
#include <lizard/file.h>
#include <nolibc/list.h>
#include <lizard/spinlock.h>
#include <nolibc/types.h>

/* Forward declarations */
struct dentry;
struct inode;
struct super_block;

#include <lizard/dentry.h>
#include <lizard/inode.h>

struct super_block {
    struct list_head list;
    const struct fs_type *type;
    struct spinlock_t lock;

    void *fs_info;

    struct dentry *root;
    u64 flags;

    size_t block_size;
    u64 total_blocks;
    u64 free_blocks;
};

struct super_ops {
    int (*statfs)(struct super_block *sb, void *out);
    int (*sync)(struct super_block *sb);
};

struct fs_type {
    struct list_head list;
    char name[32];

    struct dentry *(*mount)(struct super_block *sb, const void *data);
    void (*kill_sb)(struct super_block *sb);

    u32 flags;
    void *private_data;
};

int fs_register(struct fs_type *type);
int fs_unregister(struct fs_type *type);
struct fs_type *fs_find(const char *name);
struct fs_type *fs_find_locked(const char *name);
int fs_type_count(void);
struct fs_type *fs_detect(struct block_dev *dev);
