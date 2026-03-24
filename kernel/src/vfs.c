#include <ata.h>
#include <blkdev_manager.h>
#include <fs.h>
#include <kmalloc.h>
#include <panic.h>
#include <setup.h>
#include <stdio.h>
#include <string.h>
#include <types.h>
#include <vfs.h>

static Dentry *vfs_root;

Dentry *vfs_get_root(void)
{
    dentry_get(vfs_root);
    return vfs_root;
}

static int set_root(char *dev_str)
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

    BlockDevice *part = blkdev_manager_get_by_name(dev_str);
    if (part == NULL)
    {
        kpanic("Failed to find root device %s", dev_str);
    }

    FsType *type = fs_detect(part);
    if (type == NULL)
    {
        kpanic("Failed to detect filesystem on %s", dev_str);
    }

    SuperBlock *sb = (SuperBlock *)zalloc(sizeof(SuperBlock));
    if (sb == NULL)
    {
        kpanic("Failed to allocate SuperBlock for root");
    }

    sb->type = type;

    vfs_root = type->mount(sb, part);
    if (vfs_root == NULL)
    {
        kpanic("Failed to mount root filesystem");
    }

    kprintf("VFS: Mounted root (%s) on %s\n", type->name, dev_str);

    return 0;
}

__setup("root=", set_root);

void vfs_init()
{
}