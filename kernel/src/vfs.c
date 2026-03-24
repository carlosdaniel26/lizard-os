#include <ata.h>
#include <blkdev_manager.h>
#include <fs.h>
#include <panic.h>
#include <setup.h>
#include <stdio.h>
#include <types.h>
#include <vfs.h>

static Dentry *vfs_root;

Dentry *vfs_get_root(void)
{
    dentry_get(vfs_root);
    return vfs_root;
}

static void set_root(char *dev_str)
{
    BlockDevice *part = blkdev_manager_get_by_name(dev_str);
    if (part == NULL)
    {
        panic("Failed to find root device %s", dev_str);
    }
}

void vfs_init()
{
}