#include <ata.h>
#include <blkdev_manager.h>
#include <fs.h>
#include <init.h>
#include <kmalloc.h>
#include <panic.h>
#include <setup.h>
#include <stdio.h>
#include <string.h>
#include <types.h>
#include <vfs.h>

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

int set_root(struct block_device *dev)
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
}

void vfs_init()
{
    struct block_device *dev = blkdev_manager_get_by_name(rootdev_str);
    if (dev == NULL)
    {
        kpanic("Failed to find root device %s", rootdev_str);
    }

    setup_root(rootdev_str);
}

late_initcall(vfs_init);