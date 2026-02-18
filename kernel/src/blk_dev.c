#include <blk_dev.h>
#include <string.h>
#include <stdio.h>
#include <debug.h>

BlockDevice *blk_devs[MAX_blk_devS] = {0};
static u32 blk_dev_count = 0;

int blk_dev_register(BlockDevice *dev)
{
	if (!dev) {
		debug_printf("blk_dev_register: device is NULL\n");
		return -1;
	}
	
	if (!dev->ops) {
		debug_printf("blk_dev_register: device '%s' has no operations\n", dev->name);
		return -1;
	}
	
	if (!dev->ops->read || !dev->ops->write) {
		debug_printf("blk_dev_register: device '%s' missing read/write operations\n", dev->name);
		return -1;
	}
	
	if (blk_dev_count >= MAX_blk_devS) {
		debug_printf("blk_dev_register: maximum devices reached (%d)\n", MAX_blk_devS);
		return -1;
	}
	
	for (u32 i = 0; i < blk_dev_count; i++) 
	{
		if (strcmp(blk_devs[i]->name, dev->name) == 0) {
			debug_printf("blk_dev_register: device name '%s' already exists\n", dev->name);
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

int blk_dev_read(BlockDevice *dev, u64 sector, void *buffer, size_t count)
{
	if (!dev || !dev->ops || !dev->ops->read) {
		debug_printf("blk_dev_read: invalid device or read operation\n");
		return -1;
	}
	
	return dev->ops->read(dev, sector, buffer, count);
}

int blk_dev_write(BlockDevice *dev, u64 sector, const void *buffer, size_t count)
{
	if (!dev || !dev->ops || !dev->ops->write) {
		debug_printf("blk_dev_write: invalid device or write operation\n");
		return -1;
	}
	
	return dev->ops->write(dev, sector, buffer, count);
}