#include <lizard/blk_dev.h>
#include <lizard/debug.h>
#include <lizard/kmalloc.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>

int blk_dev_read(struct block_dev *dev, u64 sector, void *buffer, size_t count)
{
    if (!dev || !dev->ops || !dev->ops->read)
    {
        debug_printf("blk_dev_read: invalid device or read operation\n");
        return -1;
    }

    return dev->ops->read(dev, sector, buffer, count);
}

int blk_dev_write(struct block_dev *dev, u64 sector, const void *buffer, size_t count)
{
    if (!dev || !dev->ops || !dev->ops->write)
    {
        debug_printf("blk_dev_write: invalid device or write operation\n");
        return -1;
    }

    return dev->ops->write(dev, sector, buffer, count);
}

int blk_dev_part_read(struct block_dev *dev, u64 sector, void *buffer, size_t count)
{
    if (!dev || !dev->ops || !dev->ops->read)
    {
        debug_printf("blk_dev_read: invalid device or read operation\n");
        return -1;
    }

    struct partition_private *part = (struct partition_private *)dev->private_data;
    struct block_dev *parent = dev->parent;

    /* sector is an offset, bring it to the real world */
    u64 phys_sec = sector + part->start_lba;

    return parent->ops->read(parent, phys_sec, buffer, count);
}

int blk_dev_part_write(struct block_dev *dev, u64 sector, const void *buffer, size_t count)
{
    if (!dev || !dev->ops || !dev->ops->write)
    {
        debug_printf("blk_dev_write: invalid device or write operation\n");
        return -1;
    }

    struct partition_private *part = (struct partition_private *)dev->private_data;
    struct block_dev *parent = dev->parent;

    /* sector is an offset, bring it to the real world */
    u64 phys_sec = sector + part->start_lba;

    return parent->ops->write(parent, phys_sec, buffer, count);
}
