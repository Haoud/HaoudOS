#include <core/fs/file.h>
#include <core/syscall/syscall.h>

int sys_dup2(int oldfd, int newfd)
{
	struct file *open_file;
	hret_t ret;

	if (!is_valid_fd(oldfd))
		return -ERR_BAD_ARG;

	if (!is_valid_fd(newfd))
		return -ERR_BAD_ARG;

	if (newfd == oldfd)
		return newfd;

	ret = get_open_file(current, oldfd, &open_file);
	if (ret != RET_OK)
		return ret;

	if (open_file == NULL)
		return -ERR_NO_MEM;

	ref_open_file(open_file);

	// On enregistre manuellement le descripteur de fichier
	if (current->open_files[newfd] != NULL)
		unref_open_file(current->open_files[newfd]);

	current->open_files[newfd] = open_file;
	return newfd;
}