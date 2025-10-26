#pragma once

#include <stdint.h>
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
	uint32_t id;

	uint64_t total_sectors;
	uint32_t sector_size;
	uint32_t max_transfer_sectors;

	BlockDeviceOps *ops;
	void *private_data;

	bool initialized;
	bool read_only;

	uint64_t read_count;
	uint64_t write_count;

	bool present;
} BlockDevice;

extern BlockDevice *block_devs[];

/* Block layer management */
int block_dev_register(BlockDevice *dev);
int block_dev_unregister(BlockDevice *dev);
BlockDevice *block_device_find(const char *name);

/* Utility functions */
uint64_t block_dev_size(BlockDevice *dev);  /* in bytes */
bool block_dev_ready(BlockDevice *dev);

/* Public API */
int block_dev_read(BlockDevice *dev, uint64_t sector, void *buffer, size_t count);
int block_dev_write(BlockDevice *dev, uint64_t sector, const void *buffer, size_t count);