#include <core/fs/file.h>
#include <core/syscall/syscall.h>

int sys_seek(uint32_t fd, off_t *offset, int whence)
{
	struct file *to_seek;
	hret_t ret;

	ret = get_open_file(current, fd, &to_seek);
	if (ret != RET_OK)
		return ret;

	return to_seek->file_op->seek(to_seek, whence, *offset, offset);
}