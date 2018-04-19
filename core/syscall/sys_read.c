#include <core/fs/file.h>
#include <core/syscall/syscall.h>
#include <driver/keyboard/keyboard.h>

#include <lib/stdio.h>

int sys_read(uint32_t fd, const char *buf, size_t len)
{	
	char *dst_buf = (char *)buf;
	size_t len_readed = len;
	struct file *to_read;
	hret_t ret;

	ret = get_open_file(current, fd, &to_read);
	if (ret != RET_OK)
		return ret;

	if (to_read == NULL)
		return -ERR_BAD_ARG;
		
	//On lit le fichier
	if (to_read->file_op->read == NULL)
		return ERR_NOT_IMPL;
		
	ret = to_read->file_op->read(to_read, dst_buf, &len_readed);
		
	return len_readed;
}