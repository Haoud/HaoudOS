#include <lib/stdio.h>
#include <lib/string.h>
#include <core/fs/vfs.h>
#include <core/mm/heap.h>
#include <core/syscall/syscall.h>

int sys_chdir(const char *path)
{
	hret_t ret;
	char *new_cwd = NULL;
	struct inode *inode = NULL;
	char *remaning_path = NULL;

	ret = lookup_inode(path, &remaning_path, &inode);

	if (ret != RET_OK)
		return ret;

	if (inode == NULL)
		return -ERR_NOT_FOUND;

	if (remaning_path != NULL)
	{
		kfree(remaning_path);
		return -ERR_NOT_FOUND;
	}

	if (inode->type != INODE_DIRECTORY)
		return -ERR_NOT_DIR;

	parse_path(path, &new_cwd);

	// Remplacer le répertoire de travail du processus
	kfree(current->cwd_path);
	current->cwd_path = new_cwd;

	debugk("[CHDIR]: New cwd path is %s\n", new_cwd);

	return RET_OK;
}
