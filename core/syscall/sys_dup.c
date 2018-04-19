#include <core/fs/file.h>
#include <core/syscall/syscall.h>

int sys_dup(int oldfd)
{
	struct file *open_file;
	uint32_t duplicated_fd;
	hret_t ret;

	if (!is_valid_fd(oldfd))
		return -ERR_BAD_ARG;

	ret = get_open_file(current, oldfd, &open_file);
	if (ret != RET_OK)
		return ret;

	if (open_file == NULL)
		return -ERR_NO_MEM;

	ref_open_file(open_file);
	ret = register_open_file(current, open_file, &duplicated_fd);
	if (ret != RET_OK)
		return ret;

	return duplicated_fd;
}
