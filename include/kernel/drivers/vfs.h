#ifndef VFS_H
#define VFS_H

#include <stdint.h>

typedef struct VFSFile
{
    char *name;
    uint32_t size;
    uint8_t *data;
} VFSFile;

VFSFile *vfs_open(const char *name, const char *mode);
int vfs_close(VFSFile *file);

int vfs_read(VFSFile *file, void *buffer, uint32_t size);
int vfs_write(VFSFile *file, const void *buffer, uint32_t size);

int vfs_delete(const char *name);

int vfs_is_file(const char *name);
#endif
