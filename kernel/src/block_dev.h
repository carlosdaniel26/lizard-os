#pragma once

#include <types.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_BLOCK_DEVICES 16

typedef struct BlockDeviceOps {
	int (*read)();
	int (*write)();
	
	int (*flush)();
	int (*ioctl)();
} BlockDeviceOps;

typedef struct BlockDevice	{
	char name[32];
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

extern BlockDevice *block_devs[];

/* Block layer management */
int block_dev_register(BlockDevice *dev);
int block_dev_unregister(BlockDevice *dev);
BlockDevice *block_device_find(const char *name);

/* Utility functions */
u64 block_dev_size(BlockDevice *dev);  /* in bytes */
bool block_dev_ready(BlockDevice *dev);

/* Public API */
int block_dev_read(BlockDevice *dev, u64 sector, void *buffer, size_t count);
int block_dev_write(BlockDevice *dev, u64 sector, const void *buffer, size_t count);