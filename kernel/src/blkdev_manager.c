#include <blk_dev.h>
#include <blkdev_manager.h>
#include <debug.h>
#include <kmalloc.h>
#include <mbr.h>
#include <stdio.h>
#include <string.h>

#define MAX_BLKDEVS 16

static LIST_HEAD(blk_devs);

static u8 blkdev_id_bitmap[MAX_BLKDEVS / 8];

static blk_dev_t blkdev_alloc_id(void)
{
    for (int i = 0; i < MAX_BLKDEVS; i++)
    {
        int byte = i / 8;
        int bit = i % 8;

        if (!(blkdev_id_bitmap[byte] & (1 << bit)))
        {
            blkdev_id_bitmap[byte] |= (1 << bit);
            return i;
        }
    }

    return -1;
}

static void blkdev_free_id(blk_dev_t id)
{
    int byte = id / 8;
    int bit = id % 8;
    blkdev_id_bitmap[byte] &= ~(1 << bit);
}

int blkdev_manager_add(struct block_dev *dev)
{
    struct list_head *pos, *tmp;

    if (!dev)
    {
        debug_printf("blkdev_manager_add: device is NULL\n");
        return -1;
    }

    if (!dev->ops)
    {
        debug_printf("blkdev_manager_add: device '%s' has no operations\n", dev->name);
        return -1;
    }

    if (!dev->ops->read || !dev->ops->write)
    {
        debug_printf("blkdev_manager_add: device '%s' missing read/write operations\n", dev->name);
        return -1;
    }

    list_for_each(pos, tmp, &blk_devs)
    {
        struct block_dev *curr = (struct block_dev *)pos;

        if (strcmp(curr->name, dev->name) == 0)
        {
            debug_printf("blkdev_manager_add: device name '%s' already exists\n", dev->name);
            return -1;
        }
    }

    dev->id = blkdev_alloc_id();

    list_add(&dev->list, &blk_devs);

    debug_printf("Registered block device: %s (sectors: %u, size: %u bytes)\n", dev->name, dev->total_sectors,
                 dev->sector_size);

    mbr_scan(dev);

    return 0;
}

void blkdev_manager_remove(blk_dev_t id)
{
    struct list_head *pos, *tmp;
    struct block_dev *curr;

    blkdev_free_id(id);

    list_for_each(pos, tmp, &blk_devs)
    {
        curr = (struct block_dev *)pos;

        if (curr->id == id) break;
    }
}

struct block_dev *blkdev_manager_get(blk_dev_t id)
{
    struct list_head *pos, *tmp;
    struct block_dev *curr;

    list_for_each(pos, tmp, &blk_devs)
    {
        curr = (struct block_dev *)pos;

        if (curr->id == id) return curr;
    }

    return NULL;
}

struct block_dev *blkdev_manager_get_by_name(const char *name)
{
    struct list_head *pos, *tmp;
    struct block_dev *curr;

    list_for_each(pos, tmp, &blk_devs)
    {
        curr = (struct block_dev *)pos;

        if (strcmp(curr->name, name) == 0) return curr;
    }

    return NULL;
}

int blkdev_create_partition(struct block_dev *parent, int part_num, u64 start, u64 secs)
{
    struct block_dev *part = zalloc(sizeof(struct block_dev));
    struct block_dev_ops *ops = zalloc(sizeof(struct block_dev_ops));

    ops->read = blk_dev_part_read;
    ops->write = blk_dev_part_write;

    char name[DEFAULT_NAME_SIZE] = {};

    size_t len = strlen(parent->name);

    if (len > 0 && parent->name[len - 1] >= '0' && parent->name[len - 1] <= '9')
    {
        sprintf(name, "%s%c%d", parent->name, 'p', part_num);
    }
    else
    {
        sprintf(name, "%s%d", parent->name, part_num);
    }

    strcpy(part->name, name);
    part->total_sectors = secs;
    part->sector_size = parent->sector_size;
    part->ops = ops;
    part->private_data = (void *)zalloc(sizeof(struct partition_private));
    part->initialized = true;
    part->read_only = false;
    part->present = true;

    struct partition_private *part_info = (struct partition_private *)part->private_data;
    part_info->start_lba = start;
    part_info->sec_count = secs;
    part_info->parent = parent;

    blkdev_manager_add(part);

    return 0;
}
