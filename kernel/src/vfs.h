#pragma once

/* Credits to the design: https://www.cs.fsu.edu/~awang/courses/cop5611_s2024/vnode.pdf	 */

#include <types.h>

typedef struct Vfs {
    struct Vfs *next;            /* next vfs in list */
    struct VfsOps *ops;          /* operations on vfs */
    struct Vnode *vnode_covered; /* vnode we cover */
    int flags;                   /* flags */
    int block_size;              /* native block size */
    uintptr_t data;              /* private data */
} Vfs;

typedef struct VfsOps {
    int (*vfs_mount)(void);
    int (*vfs_unmount)(void);
    int (*vfs_root)(void);
    int (*vfs_statfs)(void);
    int (*vfs_sync)(void);
    int (*vfs_fid)(void);
    int (*vfs_vget)(void);
} VfsOps;

typedef enum Vtype
{
    VNON,
    VREG,
    VDIR,
    VBLK,
    VCHR,
    VLNK,
    VSOCK,
    VBAD
} Vtype;

typedef struct Vnode {
    u8 v_flag;                    /* vnode flags */
    u8 v_count;                   /* reference count */
    u8 v_shlockc;                 /* # of shared locks */
    u8 v_exlockc;                 /* # of exclusive locks */
    struct Vfs *v_vfsmountedhere; /* covering vfs */
    struct VnodeOps *v_op;        /* vnode operations */
    union
    {
        struct socket *v_Socket; /* unix ipc */
        struct stdata *v_Stream; /* stream */
    };
    struct Vfs *v_vfsp; /* vfs we are in */
    Vtype v_type;       /* vnode type */
    uintptr_t v_data;   /* private data */
} Vnode;

typedef struct VnodeOps {
    int (*vn_open)(void);
    int (*vn_close)(void);
    int (*vn_rdwr)(void);
    int (*vn_ioctl)(void);
    int (*vn_select)(void);
    int (*vn_getattr)(void);
    int (*vn_setattr)(void);
    int (*vn_access)(void);
    int (*vn_lookup)(void);
    int (*vn_create)(void);
    int (*vn_remove)(void);
    int (*vn_link)(void);
    int (*vn_rename)(void);
    int (*vn_mkdir)(void);
    int (*vn_rmdir)(void);
    int (*vn_readdir)(void);
    int (*vn_symlink)(void);
    int (*vn_readlink)(void);
    int (*vn_fsync)(void);
    int (*vn_inactive)(void);
    int (*vn_bmap)(void);
    int (*vn_strategy)(void);
    int (*vn_bread)(void);
    int (*vn_brelse)(void);
} VnodeOps;

void vfs_init(void);
