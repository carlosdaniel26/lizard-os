#pragma once

#include <atomic.h>
#include <list.h>
#include <spinlock.h>
#include <types.h>

struct inode;

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

struct dentry *dentry_alloc(const char *name);
void dentry_get(struct dentry *d);
void dentry_put(struct dentry *d);
struct dentry *dentry_lookup(struct dentry *parent, const char *name);
void dentry_add(struct dentry *parent, struct dentry *child);
