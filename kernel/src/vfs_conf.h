#ifndef VFS_CONF_H
#define VFS_CONF_H

#include <vfs.h>

typedef struct VfsConf {
    char *name;            /* name, like "ufs", "nfs" */
    int typenum;           /* internal FS type ID */
    int refcount;          /* active mounts using this FS */
    int flags;
    VfsOps ops;             /* pointer to filesystem ops */
    struct VfsConf *next;   /* next FS in the chain */
} VfsConf;

int vfs_register(VfsConf *conf);
VfsConf *vfs_byname(const char *name);

#endif