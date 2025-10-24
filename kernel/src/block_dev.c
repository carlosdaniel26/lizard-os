#include <block_dev.h>
#include <string.h>
#include <stdio.h>

BlockDevice *block_devices[MAX_BLOCK_DEVICES] = {0};
uint32_t block_device_count = 0;

int block_device_register(BlockDevice *dev)
{
	if (!dev) {
		kprintf("block_device_register: device is NULL\n");
		return -1;
	}
	
	if (!dev->ops) {
		kprintf("block_device_register: device '%s' has no operations\n", dev->name);
		return -1;
	}
	
	if (!dev->ops->read || !dev->ops->write) {
		kprintf("block_device_register: device '%s' missing read/write operations\n", dev->name);
		return -1;
	}
	
	if (block_device_count >= MAX_BLOCK_DEVICES) {
		kprintf("block_device_register: maximum devices reached (%d)\n", MAX_BLOCK_DEVICES);
		return -1;
	}
	
	for (uint32_t i = 0; i < block_device_count; i++) 
	{
		if (strcmp(block_devices[i]->name, dev->name) == 0) {
			kprintf("block_device_register: device name '%s' already exists\n", dev->name);
			return -1;
		}
	}
	
	dev->id = block_device_count;
	
	block_devices[block_device_count] = dev;
	block_device_count++;
	
	kprintf("Registered block device: %s (sectors: %u, size: %u bytes)\n",
				 dev->name, dev->total_sectors, dev->sector_size);
	
	return 0;
}

int block_device_read(BlockDevice *dev, uint64_t sector, void *buffer, size_t count)
{
	if (!dev || !dev->ops || !dev->ops->read) {
		kprintf("block_device_read: invalid device or read operation\n");
		return -1;
	}
	
	return dev->ops->read(dev, sector, buffer, count);
}

int block_device_write(BlockDevice *dev, uint64_t sector, const void *buffer, size_t count)
{
	if (!dev || !dev->ops || !dev->ops->write) {
		kprintf("block_device_write: invalid device or write operation\n");
		return -1;
	}
	
	return dev->ops->write(dev, sector, buffer, count);
}