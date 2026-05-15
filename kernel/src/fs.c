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