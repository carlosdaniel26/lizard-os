#pragma once

#include <types.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_blk_devS 16
#define DEFAULT_NAME_SIZE 32

typedef struct BlockDeviceOps {
	int (*read)();
	int (*write)();
	
	int (*flush)();
	int (*ioctl)();
} BlockDeviceOps;

typedef struct BlockDevice	{
	char name[DEFAULT_NAME_SIZE];
	u32 id;

	u64 total_sectors;
	u32 sector_size;
	u32 max_transfer_sectors;

	BlockDeviceOps *ops;
	void *private_data;

	bool initialized;
	bool read_only;

	u64 read_count;
	u64 write_count;

	bool present;
} BlockDevice;

extern BlockDevice *blk_devs[];

/* Block layer management */
int blkdev_manager_register(BlockDevice *dev);
int blk_dev_unregister(BlockDevice *dev);
BlockDevice *blk_dev_find(const char *name);

/* Utility functions */
u64 blk_dev_size(BlockDevice *dev);  /* in bytes */
bool blk_dev_ready(BlockDevice *dev);

/* Public API */
int blk_dev_read(BlockDevice *dev, u64 sector, void *buffer, size_t count);
int blk_dev_write(BlockDevice *dev, u64 sector, const void *buffer, size_t count);