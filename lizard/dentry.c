#include <lizard/dentry.h>
#include <lizard/atomic.h>
#include <lizard/fs.h>
#include <lizard/kmalloc.h>
#include <lizard/spinlock.h>
#include <nolibc/string.h>
#include <nolibc/types.h>

struct dentry *dentry_alloc(const char *name)
{
    struct dentry *d = (struct dentry *)zalloc(sizeof(struct dentry));
    if (!d) return NULL;
    strncpy(d->name, name, sizeof(d->name) - 1);
    d->name[sizeof(d->name) - 1] = '\0';
    InitListHead(&d->children);
    InitListHead(&d->sibling);
    atomic_set(&d->refcount, 1);
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
