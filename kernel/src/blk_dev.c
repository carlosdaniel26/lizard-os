#include <blk_dev.h>
#include <debug.h>
#include <kmalloc.h>
#include <stdio.h>
#include <string.h>

int blk_dev_read(BlockDevice *dev, u64 sector, void *buffer, size_t count)
{
    if (!dev || !dev->ops || !dev->ops->read)
    {
        debug_printf("blk_dev_read: invalid device or read operation\n");
        return -1;
    }

    return dev->ops->read(dev, sector, buffer, count);
}

int blk_dev_write(BlockDevice *dev, u64 sector, const void *buffer, size_t count)
{
    if (!dev || !dev->ops || !dev->ops->write)
    {
        debug_printf("blk_dev_write: invalid device or write operation\n");
        return -1;
    }

    return dev->ops->write(dev, sector, buffer, count);
}

int blk_dev_part_read(BlockDevice *dev, u64 sector, void *buffer, size_t count)
{
    if (!dev || !dev->ops || !dev->ops->read)
    {
        debug_printf("blk_dev_read: invalid device or read operation\n");
        return -1;
    }

    PartitionPrivate *part = (PartitionPrivate *)dev->private_data;
    BlockDevice *parent = part->parent;

    /* sector is an offset, bring it to the real world */
    u64 phys_sec = sector + part->start_lba;

    return part->parent->ops->read(parent, phys_sec, buffer, count);
}

int blk_dev_part_write(BlockDevice *dev, u64 sector, const void *buffer, size_t count)
{
    if (!dev || !dev->ops || !dev->ops->write)
    {
        debug_printf("blk_dev_write: invalid device or write operation\n");
        return -1;
    }

    PartitionPrivate *part = (PartitionPrivate *)dev->private_data;
    BlockDevice *parent = part->parent;

    /* sector is an offset, bring it to the real world */
    u64 phys_sec = sector + part->start_lba;

    return part->parent->ops->write(parent, phys_sec, buffer, count);
}
