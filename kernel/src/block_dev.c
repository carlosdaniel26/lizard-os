#include <block_dev.h>
#include <string.h>
#include <stdio.h>

BlockDevice *block_devs[MAX_BLOCK_DEVICES] = {0};
static u32 block_dev_count = 0;

int block_dev_register(BlockDevice *dev)
{
	if (!dev) {
		debug_printf("block_device_register: device is NULL\n");
		return -1;
	}
	
	if (!dev->ops) {
		debug_printf("block_device_register: device '%s' has no operations\n", dev->name);
		return -1;
	}
	
	if (!dev->ops->read || !dev->ops->write) {
		debug_printf("block_device_register: device '%s' missing read/write operations\n", dev->name);
		return -1;
	}
	
	if (block_dev_count >= MAX_BLOCK_DEVICES) {
		debug_printf("block_device_register: maximum devices reached (%d)\n", MAX_BLOCK_DEVICES);
		return -1;
	}
	
	for (u32 i = 0; i < block_dev_count; i++) 
	{
		if (strcmp(block_devs[i]->name, dev->name) == 0) {
			debug_printf("block_device_register: device name '%s' already exists\n", dev->name);
			return -1;
		}
	}
	
	dev->id = block_dev_count;
	
	block_devs[block_dev_count] = dev;
	block_dev_count++;
	
	debug_printf("Registered block device: %s (sectors: %u, size: %u bytes)\n",
				 dev->name, dev->total_sectors, dev->sector_size);
	
	return 0;
}

int block_dev_read(BlockDevice *dev, u64 sector, void *buffer, size_t count)
{
	if (!dev || !dev->ops || !dev->ops->read) {
		debug_printf("block_dev_read: invalid device or read operation\n");
		return -1;
	}
	
	return dev->ops->read(dev, sector, buffer, count);
}

int block_dev_write(BlockDevice *dev, u64 sector, const void *buffer, size_t count)
{
	if (!dev || !dev->ops || !dev->ops->write) {
		debug_printf("block_dev_write: invalid device or write operation\n");
		return -1;
	}
	
	return dev->ops->write(dev, sector, buffer, count);
}