#include <core/fs/file.h>
#include <core/syscall/syscall.h>

int sys_ioctl(uint32_t fd, uint32_t request_id, uint32_t request_arg)
{
	struct file *open_file;
	hret_t ret;

	ret = get_open_file(current, fd, &open_file);
	if (ret != RET_OK)
		return ret;

	if(open_file == NULL)
		return -ERR_UNKNOW;

	return open_file->char_op->ioctl(open_file, request_id, request_arg);
}