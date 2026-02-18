#include <blk_dev.h>

int blkdev_manager_add(BlockDevice *dev);
void blkdev_manager_remove(blk_dev_t id);

BlockDevice *blkdev_manager_get(blk_dev_t id);
BlockDevice *blkdev_manager_get_by_name(const char *name);
