#include <atomic.h>
#include <fs.h>
#include <list.h>
#include <spinlock.h>
#include <string.h>
#include <types.h>

LIST_HEAD(fs_types);
atomic_t fstype_count = {};

SPINLOCK(fstype_lock);

LIST_HEAD(fs_instances);

int fs_register(FsRegister *fstype)
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

int fs_unregister(FsRegister *fstype)
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

FsRegister *fs_find_locked(const char *name)
{
    ListHead *pos, *tmp;

    list_for_each(pos, tmp, &fs_types)
    {
        FsRegister *type = container_of(pos, FsRegister, list);

        if (strcmp(type->name, name) == 0)
        {
            return type;
        }
    }

    return NULL;
}

FsRegister *fs_find(const char *name)
{
    spinlock_lock(&fstype_lock);
    FsRegister *type = fs_find_locked(name);
    spinlock_unlock(&fstype_lock);

    return type;
}

int fs_type_count()
{
    return (int)atomic_read(&fstype_count);
}
