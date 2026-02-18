#pragma once

#include <types.h>
#include <stddef.h>
#include <stdbool.h>
#include <list.h>

#define DEFAULT_NAME_SIZE 32

typedef u32 blk_dev_t;

typedef struct BlockDeviceOps {
	int (*read)();
	int (*write)();
	
	int (*flush)();
	int (*ioctl)();
} BlockDeviceOps;

typedef struct BlockDevice	{
	ListHead list;
	char name[DEFAULT_NAME_SIZE];
	blk_dev_t id;

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

typedef struct
{
    BlockDevice *parent;
    u64 start_lba;
    u64 sec_count;
} PartitionPrivate;

/* Block layer management */
int blkdev_manager_add(BlockDevice *dev);
int blk_dev_unregister(BlockDevice *dev);
BlockDevice *blk_dev_find(const char *name);

/* Utility functions */
u64 blk_dev_size(BlockDevice *dev);  /* in bytes */
bool blk_dev_ready(BlockDevice *dev);

/* Public API */
int blk_dev_read(BlockDevice *dev, u64 sector, void *buffer, size_t count);
int blk_dev_write(BlockDevice *dev, u64 sector, const void *buffer, size_t count);

int blk_dev_part_read(BlockDevice *dev, u64 sector, void *buffer, size_t count);
int blk_dev_part_write(BlockDevice *dev, u64 sector, const void *buffer, size_t count);

