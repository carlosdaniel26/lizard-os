#include <blk_dev.h>
#include <blkdev_manager.h>
#include <string.h>
#include <stdio.h>
#include <debug.h>

BlockDevice *blk_devs[MAX_blk_devS] = {0};
static u32 blk_dev_count = 0;

int blkdev_manager_register(BlockDevice *dev)
{
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
	
	if (blk_dev_count >= MAX_blk_devS) {
		debug_printf("blkdev_manager_register: maximum devices reached (%d)\n", MAX_blk_devS);
		return -1;
	}
	
	for (u32 i = 0; i < blk_dev_count; i++) 
	{
		if (strcmp(blk_devs[i]->name, dev->name) == 0) {
			debug_printf("blkdev_manager_register: device name '%s' already exists\n", dev->name);
			return -1;
		}
	}
	
	dev->id = blk_dev_count;
	
	blk_devs[blk_dev_count] = dev;
	blk_dev_count++;
	
	debug_printf("Registered block device: %s (sectors: %u, size: %u bytes)\n",
				 dev->name, dev->total_sectors, dev->sector_size);
	
	return 0;
}