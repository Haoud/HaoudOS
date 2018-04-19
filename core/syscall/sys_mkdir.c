#include <core/fs/inode.h>
#include <core/syscall/syscall.h>

int sys_mkdir(const char *pathname)
{
	return create_inode(pathname, current, INODE_DIRECTORY);
}