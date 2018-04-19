#include <lib/stdio.h>
#include <core/fs/file.h>
#include <core/process/process.h>
#include <core/syscall/syscall.h>

int sys_readdir(uint32_t fd, struct dirent *direntry)
{
	struct file *to_read;
	hret_t ret;

	ret = get_open_file(current, fd, &to_read);
	if (ret != RET_OK)
		return ret;

	if (to_read == NULL)
		return -ERR_BAD_ARG;

	if (to_read->inode->type != INODE_DIRECTORY)
		return -ERR_NO_DIR;

	if (to_read->dir_op->readdir == NULL)
		return -ERR_NOT_IMPL;

	ret = to_read->dir_op->readdir(to_read, direntry);
	return ret;
}