#include <vfs_conf.h>
#include <string.h>

VfsConf *vfs_conf_list = NULL;
static int next_fs_id = 0;

int vfs_register(VfsConf *conf)
{
	conf->typenum = next_fs_id++;
	conf->next = vfs_conf_list;
	vfs_conf_list = conf;
	return 0;
}

VfsConf *vfs_byname(const char *name)
{
	for (VfsConf *c = vfs_conf_list; c; c = c->next)
		if (strcmp(c->name, name) == 0)
			return c;
	return NULL;
}