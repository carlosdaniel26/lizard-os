#pragma once

#include <lizard/fs.h>

/* open() flags */
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_CREAT  0x0040
#define O_TRUNC  0x0200 /* accepted, not yet honoured */
#define O_APPEND 0x0400

struct dentry *vfs_get_root(void);
int set_root(struct block_dev *dev);
int vfs_init();
struct dentry *vfs_lookup(struct dentry *parent, const char *name);
struct dentry *vfs_path_lookup(const char *path);

/* Fold `in` against absolute cwd `base` into a normalised absolute path in
 * `out` (see vfs.c). Returns 0, or -1 if it doesn't fit. */
int vfs_resolve_path(const char *base, const char *in, char *out, size_t outsz);
void *vfs_read_all(const char *path);

/* Namespace operations (absolute paths). Return 0 on success, -1 on failure. */
int vfs_create(const char *path, int mode);
int vfs_mkdir(const char *path, int mode);
int vfs_unlink(const char *path);
int vfs_rmdir(const char *path);

/* Open-file API. */
struct file *vfs_open(const char *path, int flags);
void vfs_close(struct file *file);
ssize_t vfs_read(struct file *file, void *buf, size_t count);
ssize_t vfs_write(struct file *file, const void *buf, size_t count);
