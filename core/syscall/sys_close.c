#include <lib/stdio.h>
#include <core/fs/file.h>
#include <core/syscall/syscall.h>

int sys_close(uint32_t fd)
{
	struct file *to_close;
	hret_t ret;

	debugk("[CLOSE]: Closing fd %u\n", fd);

	ret = get_open_file(current, fd, &to_close);
	if (ret != RET_OK)
		return ret;

	if (to_close == NULL)
	{
		debugk("[CLOSE]: Tentative de fermeture d'un fichier invalide (fd %u)\n", fd);
		return -ERR_BAD_ARG;
	}

	unregister_open_file(current, fd);
	unref_open_file(to_close);

	return RET_OK;
}