#include <lizard/inode.h>
#include <lizard/fs.h>
#include <lizard/kmalloc.h>
#include <nolibc/types.h>

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
