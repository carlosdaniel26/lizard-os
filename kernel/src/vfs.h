#ifndef VFS_H
#define VFS_H

/* Credits to the design: https://www.cs.fsu.edu/~awang/courses/cop5611_s2024/vnode.pdf  */

#include <stdint.h>

typedef struct Vfs {
    struct Vfs *next;              /* next vfs in list */
    struct VfsOps *ops;             /* operations on vfs */
    struct Vnode *vnode_covered;    /* vnode we cover */
    int flags;                      /* flags */
    int block_size;                     /* native block size */
    uintptr_t data;                  /* private data */
} Vfs;

typedef struct VfsOps {
    int (*vfs_mount)();
    int (*vfs_unmount)();
    int (*vfs_root)();
    int (*vfs_statfs)();
    int (*vfs_sync)();
    int (*vfs_fid)();
    int (*vfs_vget)();
} VfsOps;

typedef enum Vtype { VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VBAD } Vtype;

typedef struct Vnode {
    uint8_t v_flag;                   /* vnode flags */
    uint8_t v_count;                  /* reference count */
    uint8_t v_shlockc;                /* # of shared locks */
    uint8_t v_exlockc;                /* # of exclusive locks */
    struct Vfs *v_vfsmountedhere;     /* covering vfs */
    struct VnodeOps *v_op;            /* vnode operations */
    union {
        struct socket *v_Socket;      /* unix ipc */
        struct stdata *v_Stream;      /* stream */
    };
    struct Vfs *v_vfsp;               /* vfs we are in */
    Vtype v_type;                     /* vnode type */
    uintptr_t v_data;                   /* private data */
} Vnode;

typedef struct VnodeOps {
    int (*vn_open)();
    int (*vn_close)();
    int (*vn_rdwr)();
    int (*vn_ioctl)();
    int (*vn_select)();
    int (*vn_getattr)();
    int (*vn_setattr)();
    int (*vn_access)();
    int (*vn_lookup)();
    int (*vn_create)();
    int (*vn_remove)();
    int (*vn_link)();
    int (*vn_rename)();
    int (*vn_mkdir)();
    int (*vn_rmdir)();
    int (*vn_readdir)();
    int (*vn_symlink)();
    int (*vn_readlink)();
    int (*vn_fsync)();
    int (*vn_inactive)();
    int (*vn_bmap)();
    int (*vn_strategy)();
    int (*vn_bread)();
    int (*vn_brelse)();
} VnodeOps;

#endif