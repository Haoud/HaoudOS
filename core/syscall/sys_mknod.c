#include <types.h>
#include <core/dev/dev.h>
#include <core/fs/inode.h>
#include <core/syscall/syscall.h>

int sys_mknod(const char *path, mode_t mode, dev_t devid)
{
	if (mode & S_IFREG)
	{
		return create_inode(path, current, INODE_FILE);
	}
	else if (mode & S_IFDIR)
	{
		return create_inode(path, current, INODE_DIRECTORY);
	}
	else if (mode & S_IFCHR)
	{
		return make_speacial_node(path, current, INODE_CHAR_DEVICE, devid);
	}
	else if (mode & S_IFBLK)
	{
		return make_speacial_node(path, current, INODE_BLOCK_DEVICE, devid);
	}

	return -ERR_BAD_ARG;
}