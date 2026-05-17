#pragma once

#include <fs.h>

struct dentry *vfs_get_root(void);
int set_root(struct block_dev *dev);
int vfs_init();
struct dentry *vfs_lookup(struct dentry *parent, const char *name);
void *vfs_read_all(const char *path);
