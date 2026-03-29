#include <blk_dev.h>

int blkdev_manager_add(struct block_device *dev);
void blkdev_manager_remove(blk_dev_t id);

struct block_device *blkdev_manager_get(blk_dev_t id);
struct block_device *blkdev_manager_get_by_name(const char *name);

/* Partittions */
int blkdev_create_partition(struct block_device *parent, int part_num, u64 start, u64 secs);
