#include <lizard/ata.h>
#include <lizard/blkdev_manager.h>
#include <lizard/fs.h>
#include <lizard/init.h>
#include <lizard/kmalloc.h>
#include <lizard/panic.h>
#include <lizard/setup.h>
#include <nolibc/stdbool.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>
#include <nolibc/types.h>
#include <lizard/vfs.h>

__initdata char rootdev_str[64] = {0};
static struct dentry *vfs_root;

struct dentry *vfs_get_root(void)
{
    dentry_get(vfs_root);
    return vfs_root;
}

static int setup_root(char *dev_str)
{
    if (strncmp(dev_str, "UUID=", sizeof("UUID=") - 1) == 0)
    {
        kpanic("UUID-based root device specification is not supported yet");
    }
    else if (strncmp(dev_str, "PARTUUID=", sizeof("PARTUUID=") - 1) == 0)
    {
        kpanic("PARTUUID-based root device specification is not supported yet");
    }
    else if (strncmp(dev_str, "LABEL=", sizeof("LABEL=") - 1) == 0)
    {
        kpanic("LABEL-based root device specification is not supported yet");
    }

    strncpy(rootdev_str, dev_str, sizeof(rootdev_str) - 1);

    return 0;
}

__setup("root=", setup_root);

int set_root(struct block_dev *dev)
{
    if (dev == NULL)
    {
        kpanic("Root device is NULL");
    }

    struct fs_type *type = fs_detect(dev);
    if (type == NULL)
    {
        kpanic("Failed to detect filesystem on %s", dev->name);
    }

    struct super_block *sb = (struct super_block *)zalloc(sizeof(struct super_block));
    if (sb == NULL)
    {
        kpanic("Failed to allocate struct super_block for root");
    }

    sb->type = type;

    vfs_root = type->mount(sb, dev);
    if (vfs_root == NULL)
    {
        kpanic("Failed to mount root filesystem");
    }

    kprintf("VFS: Mounted root (%s) on %s\n", type->name, dev->name);
    return 0;
}

int vfs_init()
{
    struct block_dev *dev = blkdev_manager_get_by_name(rootdev_str);
    if (dev == NULL)
    {
        kpanic("Failed to find root device %s", rootdev_str);
    }

    set_root(dev);
    return 0;
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

struct dentry *vfs_path_lookup(const char *path)
{
    if (!path || *path == '\0') return NULL;

    struct dentry *current_dentry;
    if (*path == '/')
    {
        current_dentry = vfs_get_root();
        while (*path == '/') path++;
    }
    else
    {
        // TODO: Support relative paths in here when CWD is implemented
        current_dentry = vfs_get_root();
    }

    if (*path == '\0') return current_dentry;

    char name[NAME_MAX];
    while (*path)
    {
        size_t i = 0;
        while (*path && *path != '/' && i < NAME_MAX - 1)
        {
            name[i++] = *path++;
        }
        name[i] = '\0';

        while (*path == '/') path++;

        struct dentry *next = vfs_lookup(current_dentry, name);
        dentry_put(current_dentry);

        if (!next) return NULL;
        current_dentry = next;
    }

    return current_dentry;
}

/*
 * Resolve every component of `path` except the last, returning the parent
 * dentry (one reference held, release with dentry_put) and copying the final
 * component into `leaf`. Absolute paths only.
 */
static struct dentry *vfs_lookup_parent(const char *path, char *leaf, size_t leaf_sz)
{
    if (!path || *path != '/') return NULL;

    while (*path == '/') path++;
    if (*path == '\0') return NULL; /* "/" has no leaf */

    struct dentry *cur = vfs_get_root();
    const char *seg = path;

    for (;;)
    {
        const char *seg_end = seg;
        while (*seg_end && *seg_end != '/') seg_end++;

        const char *next = seg_end;
        while (*next == '/') next++;

        size_t n = (size_t)(seg_end - seg);

        if (*next == '\0')
        {
            if (n == 0 || n >= leaf_sz)
            {
                dentry_put(cur);
                return NULL;
            }
            memcpy(leaf, seg, n);
            leaf[n] = '\0';
            return cur;
        }

        char name[NAME_MAX];
        if (n == 0 || n >= sizeof(name))
        {
            dentry_put(cur);
            return NULL;
        }
        memcpy(name, seg, n);
        name[n] = '\0';

        struct dentry *child = vfs_lookup(cur, name);
        dentry_put(cur);
        if (!child) return NULL;

        cur = child;
        seg = next;
    }
}

static int vfs_mknod(const char *path, int mode, bool dir)
{
    char leaf[NAME_MAX];
    struct dentry *parent = vfs_lookup_parent(path, leaf, sizeof(leaf));
    if (!parent) return -1;

    int rc = -1;
    struct inode *pi = parent->inode;
    int (*op)(struct inode *, struct dentry *, int) =
        pi && pi->i_ops ? (dir ? pi->i_ops->mkdir : pi->i_ops->create) : NULL;

    if (op && !dentry_lookup(parent, leaf))
    {
        struct dentry *d = dentry_alloc(leaf);
        if (d)
        {
            rc = op(pi, d, mode);
            if (rc == 0)
                dentry_add(parent, d);
            else
                dentry_put(d);
        }
    }

    dentry_put(parent);
    return rc;
}

int vfs_create(const char *path, int mode)
{
    return vfs_mknod(path, mode, false);
}

int vfs_mkdir(const char *path, int mode)
{
    return vfs_mknod(path, mode, true);
}

static int vfs_remove(const char *path, bool dir)
{
    char leaf[NAME_MAX];
    struct dentry *parent = vfs_lookup_parent(path, leaf, sizeof(leaf));
    if (!parent) return -1;

    int rc = -1;
    struct inode *pi = parent->inode;
    int (*op)(struct inode *, struct dentry *) =
        pi && pi->i_ops ? (dir ? pi->i_ops->rmdir : pi->i_ops->unlink) : NULL;

    if (op)
    {
        struct dentry *d = dentry_alloc(leaf);
        if (d)
        {
            rc = op(pi, d);
            if (rc == 0)
            {
                struct dentry *cached = dentry_lookup(parent, leaf);
                if (cached)
                {
                    list_del(&cached->sibling);
                    dentry_put(cached); /* drop the lookup ref */
                    dentry_put(cached); /* drop the cache ref */
                }
            }
            dentry_put(d);
        }
    }

    dentry_put(parent);
    return rc;
}

int vfs_unlink(const char *path)
{
    return vfs_remove(path, false);
}

int vfs_rmdir(const char *path)
{
    return vfs_remove(path, true);
}

struct file *vfs_open(const char *path, int flags)
{
    struct dentry *d = vfs_path_lookup(path);

    if (!d && (flags & O_CREAT))
    {
        if (vfs_create(path, 0) != 0) return NULL;
        d = vfs_path_lookup(path);
    }
    if (!d) return NULL;

    if (!d->inode)
    {
        dentry_put(d);
        return NULL;
    }

    struct file *file = zalloc(sizeof(struct file));
    if (!file)
    {
        dentry_put(d);
        return NULL;
    }

    file->inode = d->inode;
    file->offset = 0;
    file->flags = flags;
    file->private_data = d; /* keeps the dentry ref until vfs_close() */

    if (file->inode->f_ops && file->inode->f_ops->open)
    {
        if (file->inode->f_ops->open(file->inode, file) != 0)
        {
            dentry_put(d);
            kfree(file);
            return NULL;
        }
    }

    return file;
}

void vfs_close(struct file *file)
{
    if (!file) return;

    if (file->inode && file->inode->f_ops && file->inode->f_ops->release)
        file->inode->f_ops->release(file->inode, file);

    if (file->private_data)
        dentry_put((struct dentry *)file->private_data);

    kfree(file);
}

ssize_t vfs_read(struct file *file, void *buf, size_t count)
{
    if (!file || !file->inode || !file->inode->f_ops || !file->inode->f_ops->read)
        return -1;

    ssize_t n = file->inode->f_ops->read(file, buf, count, (off_t)file->offset);
    if (n > 0) file->offset += (u64)n;
    return n;
}

ssize_t vfs_write(struct file *file, const void *buf, size_t count)
{
    if (!file || !file->inode || !file->inode->f_ops || !file->inode->f_ops->write)
        return -1;

    if (file->flags & O_APPEND)
        file->offset = file->inode->size;

    ssize_t n = file->inode->f_ops->write(file, buf, count, (off_t)file->offset);
    if (n > 0) file->offset += (u64)n;
    return n;
}

late_initcall(vfs_init);