#include <inode.h>
#include <fs.h>
#include <kmalloc.h>
#include <types.h>

struct inode *inode_alloc(struct super_block *sb)
{
    struct inode *inode = (struct inode *)zalloc(sizeof(struct inode));
    if (!inode) return NULL;
    inode->sb = sb;
    return inode;
}

void inode_free(struct inode *inode)
{
    if (!inode) return;
    kfree(inode);
}
