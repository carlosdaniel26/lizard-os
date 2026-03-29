#pragma once

#include <list.h>
#include <stdbool.h>
#include <types.h>

#define DEFAULT_NAME_SIZE 32

typedef u32 blk_dev_t;

struct block_device_ops {
    int (*read)();
    int (*write)();

    int (*flush)();
    int (*ioctl)();
};

struct block_device {
    struct list_head list;
    char name[DEFAULT_NAME_SIZE];
    blk_dev_t id;

    u64 total_sectors;
    u32 sector_size;
    u32 max_transfer_sectors;

    struct block_device_ops *ops;
    void *private_data;

    bool initialized;
    bool read_only;

    u64 read_count;
    u64 write_count;

    bool present;
};

struct partition_private {
    struct block_device *parent;
    u64 start_lba;
    u64 sec_count;
};

/* Block layer management */
int blkdev_manager_add(struct block_device *dev);
int blk_dev_unregister(struct block_device *dev);
struct block_device *blk_dev_find(const char *name);

/* Utility functions */
u64 blk_dev_size(struct block_device *dev); /* in bytes */
bool blk_dev_ready(struct block_device *dev);

/* Public API */
int blk_dev_read(struct block_device *dev, u64 sector, void *buffer, size_t count);
int blk_dev_write(struct block_device *dev, u64 sector, const void *buffer, size_t count);

int blk_dev_part_read(struct block_device *dev, u64 sector, void *buffer, size_t count);
int blk_dev_part_write(struct block_device *dev, u64 sector, const void *buffer, size_t count);
