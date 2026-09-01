#include <lizard/blk_dev.h>
#include <lizard/debug.h>
#include <lizard/kmalloc.h>
#include <nolibc/stdbool.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>

/*
 * Small write-through sector cache. Only single-sector, 512-byte transfers are
 * cached; larger or odd-sized transfers bypass it (and invalidate any lines
 * they overlap on write). No locking - matches the rest of the block layer.
 */

#define BCACHE_LINES       64
#define BCACHE_SECTOR_SIZE 512

struct bcache_line {
    struct block_dev *dev;
    u64 sector;
    u64 lru;
    bool valid;
    u8 data[BCACHE_SECTOR_SIZE];
};

static struct bcache_line bcache[BCACHE_LINES];
static u64 bcache_clock;

static bool bcache_eligible(struct block_dev *dev, size_t count)
{
    return count == 1 && (dev->sector_size == 0 || dev->sector_size == BCACHE_SECTOR_SIZE);
}

static struct bcache_line *bcache_find(struct block_dev *dev, u64 sector)
{
    for (int i = 0; i < BCACHE_LINES; i++)
        if (bcache[i].valid && bcache[i].dev == dev && bcache[i].sector == sector)
            return &bcache[i];
    return NULL;
}

static struct bcache_line *bcache_victim(void)
{
    struct bcache_line *victim = &bcache[0];
    for (int i = 1; i < BCACHE_LINES; i++)
    {
        if (!bcache[i].valid) return &bcache[i];
        if (bcache[i].lru < victim->lru) victim = &bcache[i];
    }
    return victim;
}

static void bcache_store(struct block_dev *dev, u64 sector, const void *data)
{
    struct bcache_line *line = bcache_find(dev, sector);
    if (!line) line = bcache_victim();

    line->dev = dev;
    line->sector = sector;
    line->valid = true;
    line->lru = ++bcache_clock;
    memcpy(line->data, data, BCACHE_SECTOR_SIZE);
}

static void bcache_invalidate_range(struct block_dev *dev, u64 sector, size_t count)
{
    for (int i = 0; i < BCACHE_LINES; i++)
        if (bcache[i].valid && bcache[i].dev == dev &&
            bcache[i].sector >= sector && bcache[i].sector < sector + count)
            bcache[i].valid = false;
}

int blk_dev_read(struct block_dev *dev, u64 sector, void *buffer, size_t count)
{
    if (!dev || !dev->ops || !dev->ops->read)
    {
        debug_printf("blk_dev_read: invalid device or read operation\n");
        return -1;
    }

    if (!bcache_eligible(dev, count))
        return dev->ops->read(dev, sector, buffer, count);

    struct bcache_line *line = bcache_find(dev, sector);
    if (line)
    {
        line->lru = ++bcache_clock;
        memcpy(buffer, line->data, BCACHE_SECTOR_SIZE);
        return 0;
    }

    if (dev->ops->read(dev, sector, buffer, count) != 0)
        return -1;

    bcache_store(dev, sector, buffer);
    return 0;
}

int blk_dev_write(struct block_dev *dev, u64 sector, const void *buffer, size_t count)
{
    if (!dev || !dev->ops || !dev->ops->write)
    {
        debug_printf("blk_dev_write: invalid device or write operation\n");
        return -1;
    }

    if (dev->ops->write(dev, sector, buffer, count) != 0)
        return -1;

    if (bcache_eligible(dev, count))
        bcache_store(dev, sector, buffer);
    else
        bcache_invalidate_range(dev, sector, count);

    return 0;
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
