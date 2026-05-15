#pragma once

#include <atomic.h>
#include <blkdev_manager.h>
#include <list.h>
#include <spinlock.h>
#include <types.h>

/* Forward declarations */
struct dentry;
struct inode;
struct super_block;

#include <dentry.h>
#include <inode.h>

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

struct file {
    struct inode *inode;
    u64 offset;
    u32 flags;
    void *private_data;
};

struct file_ops {
    int (*open)(struct inode *inode, struct file *file);
    ssize_t (*read)(struct file *file, char *buf, size_t count, off_t offset);
    ssize_t (*write)(struct file *file, const char *buf, size_t count, off_t offset);
    int (*readdir)(struct file *file, void *dirent, int (*filldir)(void *, const char *, int, off_t, u64));
    int (*release)(struct inode *inode, struct file *file);
};

int fs_register(struct fs_type *type);
int fs_unregister(struct fs_type *type);
struct fs_type *fs_find(const char *name);
struct fs_type *fs_find_locked(const char *name);
int fs_type_count(void);
struct fs_type *fs_detect(struct block_dev *dev);
