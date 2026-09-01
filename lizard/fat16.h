#pragma once

#include <lizard/blk_dev.h>
#include <lizard/fat16_types.h>
#include <lizard/fs.h>

int fat16_detect(struct block_dev *dev);
int fat16_mount(struct block_dev *dev, struct fat16 *fs);
int fat16_init();

struct dentry *fat16_mount_fs(struct super_block *sb, const void *data);
