#include <atomic.h>
#include <fs.h>
#include <kmalloc.h>
#include <list.h>
#include <spinlock.h>
#include <string.h>
#include <types.h>

LIST_HEAD(fs_types);
struct atomic_t fstype_count = {};

SPINLOCK(fstype_lock);

LIST_HEAD(fs_instances);

int fs_register(struct fs_type *fstype)
{
    if (!fstype || !fstype->name[0])
    {
        return 0;
    }

    spinlock_lock(&fstype_lock);

    if (fs_find_locked(fstype->name))
    {
        spinlock_unlock(&fstype_lock);
        return -1;
    }

    list_add(&fstype->list, &fs_types);
    atomic_inc(&fstype_count);

    spinlock_unlock(&fstype_lock);

    return 0;
}

int fs_unregister(struct fs_type *fstype)
{
    if (!fstype)
    {
        return 0;
    }

    spinlock_lock(&fstype_lock);

    list_del(&fstype->list);
    atomic_dec(&fstype_count);

    spinlock_unlock(&fstype_lock);

    return 0;
}

struct fs_type *fs_find_locked(const char *name)
{
    struct list_head *pos, *tmp;

    list_for_each(pos, tmp, &fs_types)
    {
        struct fs_type *type = container_of(pos, struct fs_type, list);

        if (strcmp(type->name, name) == 0)
        {
            return type;
        }
    }

    return NULL;
}

struct fs_type *fs_find(const char *name)
{
    spinlock_lock(&fstype_lock);
    struct fs_type *type = fs_find_locked(name);
    spinlock_unlock(&fstype_lock);

    return type;
}

int fs_type_count()
{
    return (int)atomic_read(&fstype_count);
}

struct fs_type *fs_detect(struct block_dev *dev)
{
    struct list_head *pos, *tmp;

    list_for_each(pos, tmp, &fs_types)
    {
        struct fs_type *type = container_of(pos, struct fs_type, list);

        return type;
    }

    return NULL;
}

struct inode *inode_alloc(struct super_block *sb)
{
    struct inode *inode = (struct inode *)zalloc(sizeof(struct inode));
    if (!inode) return NULL;
    inode->sb = sb;
    return inode;
}

void inode_free(struct inode *inode)
{
    if (!inode) return;
    kfree(inode);
}

struct dentry *dentry_alloc(const char *name)
{
    struct dentry *d = (struct dentry *)zalloc(sizeof(struct dentry));
    if (!d) return NULL;
    strncpy(d->name, name, sizeof(d->name) - 1);
    d->name[sizeof(d->name) - 1] = '\0';
    return d;
}

void dentry_get(struct dentry *d)
{
    if (d) atomic_inc(&d->refcount);
}

void dentry_put(struct dentry *d)
{
    if (!d) return;
    if (atomic_dec_and_test(&d->refcount))
    {
        kfree(d);
    }
}

void dentry_add(struct dentry *parent, struct dentry *child)
{
    if (!parent || !child) return;
    spinlock_lock(&parent->lock);
    list_add_tail(&child->sibling, &parent->children);
    child->parent = parent;
    spinlock_unlock(&parent->lock);
}

struct dentry *dentry_lookup(struct dentry *parent, const char *name)
{
    if (!parent) return NULL;
    spinlock_lock(&parent->lock);
    struct list_head *pos, *tmp;
    list_for_each(pos, tmp, &parent->children)
    {
        struct dentry *d = container_of(pos, struct dentry, sibling);
        if (strcmp(d->name, name) == 0)
        {
            dentry_get(d);
            spinlock_unlock(&parent->lock);
            return d;
        }
    }
    spinlock_unlock(&parent->lock);
    return NULL;
}

struct dentry *vfs_lookup(struct dentry *parent, const char *name)
{
    struct dentry *d = dentry_lookup(parent, name);
    if (d) return d;

    d = dentry_alloc(name);
    if (!d) return NULL;

    if (parent->inode && parent->inode->i_ops && parent->inode->i_ops->lookup)
    {
        if (parent->inode->i_ops->lookup(parent->inode, d) == 0)
        {
            dentry_add(parent, d);
            return d;
        }
    }

    dentry_put(d);
    return NULL;
}