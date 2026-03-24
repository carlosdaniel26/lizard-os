#include <ata.h>
#include <panic.h>
#include <setup.h>
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

static int root_setup(char *val)
{
    (char *)val;
    kprintf("root = %s\n", val);
    return 1;
}

static int debug_setup(char *val)
{
    (char *)val;

    kprintf("debug enabled\n");
    while (1)
        ;
    return 1;
}

__setup("root=", root_setup);
__setup("debug", debug_setup);