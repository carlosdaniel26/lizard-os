#include <blk_dev.h>
#include <blkdev_manager.h>
#include <string.h>
#include <stdio.h>
#include <debug.h>

#define MAX_BLKDEVS 16

static LIST_HEAD(blk_devs);

static u8 blkdev_id_bitmap[MAX_BLKDEVS / 8];

static int blkdev_alloc_id(void)
{
    for (int i = 0; i < MAX_BLKDEVS; i++)
    {
        int byte = i / 8;
        int bit  = i % 8;

        if (!(blkdev_id_bitmap[byte] & (1 << bit)))
        {
            blkdev_id_bitmap[byte] |= (1 << bit);
            return i;
        }
    }

    return -1;
}

static void blkdev_free_id(int id)
{
    int byte = id / 8;
    int bit  = id % 8;
    blkdev_id_bitmap[byte] &= ~(1 << bit);
}

int blkdev_manager_register(BlockDevice *dev)
{
    ListHead *pos, *tmp;

	if (!dev) {
		debug_printf("blkdev_manager_register: device is NULL\n");
		return -1;
	}
	
	if (!dev->ops) {
		debug_printf("blkdev_manager_register: device '%s' has no operations\n", dev->name);
		return -1;
	}
	
	if (!dev->ops->read || !dev->ops->write) {
		debug_printf("blkdev_manager_register: device '%s' missing read/write operations\n", dev->name);
		return -1;
	}

	
	list_for_each(pos, tmp, &blk_devs)
	{
        BlockDevice *curr = (BlockDevice*)pos;

		if (strcmp(curr->name, dev->name) == 0) {
			debug_printf("blkdev_manager_register: device name '%s' already exists\n", dev->name);
			return -1;
		}
	}
	
	dev->id = blkdev_alloc_id();
	
	list_add(dev, &blk_devs);
	
	debug_printf("Registered block device: %s (sectors: %u, size: %u bytes)\n",
				 dev->name, dev->total_sectors, dev->sector_size);
	
	return 0;
}