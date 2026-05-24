#include <lizard/file.h>
#include <lizard/kmalloc.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>
#include <lizard/vfs.h>

void *vfs_read_all(const char *path)
{
    struct dentry *dentry = vfs_path_lookup(path);
    if (!dentry)
    {
        kprintf("VFS: Failed to lookup %s\n", path);
        return NULL;
    }

    struct inode *inode = dentry->inode;
    if (!inode)
    {
        dentry_put(dentry);
        return NULL;
    }

    struct file file = {.inode = inode, .offset = 0};

    if (inode->f_ops && inode->f_ops->open)
    {
        if (inode->f_ops->open(inode, &file) != 0)
        {
            kprintf("VFS: Failed to open %s\n", path);
            goto out;
        }
    }

    void *buffer = zalloc(inode->size);
    if (inode->f_ops && inode->f_ops->read)
    {
        ssize_t bytes_read = inode->f_ops->read(&file, buffer, inode->size, 0);
        if (bytes_read != (ssize_t)inode->size)
        {
            kprintf("VFS: Failed to read %s\n", path);
            kfree(buffer);
            buffer = NULL;
            goto out;
        }
    }

out:
    dentry_put(dentry);
    return buffer;
}
