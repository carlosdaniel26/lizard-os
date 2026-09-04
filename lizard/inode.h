#pragma once

#include <nolibc/types.h>

struct super_block;
struct dentry;
struct inode;

struct inode_ops {
    int (*lookup)(struct inode *dir, struct dentry *dentry);
    int (*create)(struct inode *dir, struct dentry *dentry, int mode);
    int (*mkdir)(struct inode *dir, struct dentry *dentry, int mode);
    int (*unlink)(struct inode *dir, struct dentry *dentry);
    int (*rmdir)(struct inode *dir, struct dentry *dentry);
    int (*rename)(struct inode *old, struct inode *new);
    int (*truncate)(struct inode *inode, u64 length); /* only length 0 for now */
};

struct inode {
    u32 mode;
    u64 size;

    struct inode_ops *i_ops;
    struct file_ops *f_ops;

    struct super_block *sb;
    void *private_data;
};

struct inode *inode_alloc(struct super_block *sb);
void inode_free(struct inode *inode);
