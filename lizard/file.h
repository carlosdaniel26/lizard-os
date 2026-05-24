#pragma once

#include <nolibc/dirent.h>
#include <lizard/inode.h>
#include <nolibc/types.h>

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
    int (*readdir)(struct file *file, void *dirent, int (*filldir)(void *, struct dirent *));
    int (*release)(struct inode *inode, struct file *file);
};
