#pragma once

#include <atomic.h>
#include <blkdev_manager.h>
#include <list.h>
#include <spinlock.h>
#include <types.h>




struct dentry {
    struct list_head sibling;
    struct list_head children;

    char name[NAME_MAX];
    u32 name_len;

    struct dentry *parent;

    struct inode *inode;

    struct spinlock_t lock;
    struct atomic_t refcount;

    u32 flags;
};

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

struct inode_ops {
    int (*lookup)(struct inode *dir, struct dentry *dentry);
    int (*create)(struct inode *dir, struct dentry *dentry, int mode);
    int (*mkdir)(struct inode *dir, struct dentry *dentry, int mode);
    int (*unlink)(struct inode *dir, struct dentry *dentry);
    int (*rename)(struct inode *old, struct inode *new);
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

struct inode {
    u32 mode;
    u64 size;

    struct inode_ops *i_ops;
    struct file_ops *f_ops;

    struct super_block *sb;
    void *private_data;
};

int fs_register(struct fs_type *type);
int fs_unregister(struct fs_type *type);
struct fs_type *fs_find(const char *name);
struct fs_type *fs_find_locked(const char *name);
int fs_type_count(void);
struct fs_type *fs_detect(struct block_dev *dev);

struct dentry *dentry_alloc(const char *name);
void dentry_get(struct dentry *d);
void dentry_put(struct dentry *d);
struct dentry *dentry_lookup(struct dentry *parent, const char *name);
void dentry_add(struct dentry *parent, struct dentry *child);

struct inode *inode_alloc(struct super_block *sb);
void inode_free(struct inode *inode);