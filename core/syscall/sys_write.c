#include <core/fs/file.h>
#include <driver/vga/text.h>
#include <core/syscall/syscall.h>

#include <lib/stdio.h>
#include <driver/bochs/bochs.h>

int sys_write(uint32_t fd, const char *buf, size_t len)
{
	char *src_buf = (char *)buf;
	size_t len_writed = len;
	struct file *to_write;
	hret_t ret;

	ret = get_open_file(current, fd, &to_write);
	if (ret != RET_OK)
		return ret;

	if (to_write == NULL)
		return -ERR_BAD_ARG;

	//On lit le fichier
	if (to_write->file_op->write == NULL)
		return -ERR_NOT_IMPL;

	ret = to_write->file_op->write(to_write, src_buf, &len_writed);
	if(ret != RET_OK)
		return ret;

	return len_writed;
}