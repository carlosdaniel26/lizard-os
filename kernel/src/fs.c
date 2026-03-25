#include <atomic.h>
#include <fs.h>
#include <kmalloc.h>
#include <list.h>
#include <spinlock.h>
#include <string.h>
#include <types.h>

LIST_HEAD(fs_types);
atomic_t fstype_count = {};

SPINLOCK(fstype_lock);

LIST_HEAD(fs_instances);

int fs_register(FsType *fstype)
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

int fs_unregister(FsType *fstype)
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

FsType *fs_find_locked(const char *name)
{
    ListHead *pos, *tmp;

    list_for_each(pos, tmp, &fs_types)
    {
        FsType *type = container_of(pos, FsType, list);

        if (strcmp(type->name, name) == 0)
        {
            return type;
        }
    }

    return NULL;
}

FsType *fs_find(const char *name)
{
    spinlock_lock(&fstype_lock);
    FsType *type = fs_find_locked(name);
    spinlock_unlock(&fstype_lock);

    return type;
}

int fs_type_count()
{
    return (int)atomic_read(&fstype_count);
}

FsType *fs_detect(BlockDevice *dev)
{
    ListHead *pos, *tmp;

    list_for_each(pos, tmp, &fs_types)
    {
        FsType *type = container_of(pos, FsType, list);

        return type;
    }

    return NULL;
}

Inode *inode_alloc(SuperBlock *sb)
{
    Inode *inode = (Inode *)zalloc(sizeof(Inode));
    if (!inode) return NULL;
    inode->sb = sb;
    return inode;
}

void inode_free(Inode *inode)
{
    if (!inode) return;
    kfree(inode);
}

Dentry *dentry_alloc(const char *name)
{
    Dentry *d = (Dentry *)zalloc(sizeof(Dentry));
    if (!d) return NULL;
    strncpy(d->name, name, sizeof(d->name) - 1);
    d->name[sizeof(d->name) - 1] = '\0';
    return d;
}

void dentry_get(Dentry *d)
{
    if (d) atomic_inc(&d->refcount);
}

void dentry_put(Dentry *d)
{
    if (!d) return;
    if (atomic_dec_and_test(&d->refcount))
    {
        kfree(d);
    }
}

void dentry_add(Dentry *parent, Dentry *child)
{
    if (!parent || !child) return;
    spinlock_lock(&parent->lock);
    list_add_tail(&child->sibling, &parent->children);
    child->parent = parent;
    spinlock_unlock(&parent->lock);
}

Dentry *dentry_lookup(Dentry *parent, const char *name)
{
    if (!parent) return NULL;
    spinlock_lock(&parent->lock);
    ListHead *pos, *tmp;
    list_for_each(pos, tmp, &parent->children)
    {
        Dentry *d = container_of(pos, Dentry, sibling);
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

Dentry *vfs_lookup(Dentry *parent, const char *name)
{
    Dentry *d = dentry_lookup(parent, name);
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