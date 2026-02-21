#include <ata.h>
#include <panic.h>
#include <stdio.h>
#include <types.h>
#include <vfs.h>
#include <vfs_conf.h>

VfsConf *vfs_conf_list;

static Vfs root = {0};

void vfs_init()
{
    /* Setup root */
    if (vfs_conf_list == NULL) kpanic("NO ROOT TO MOUNT");

    root.ops = &vfs_conf_list->ops;
    root.ops->vfs_mount(&root, "/", NULL);
}