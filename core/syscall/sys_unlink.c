#include <core/fs/vfs.h>
#include <core/mm/heap.h>
#include <core/fs/inode.h>
#include <core/syscall/syscall.h>

int sys_unlink(const char *path)
{
	struct inode *to_remove;
	struct inode *parent;
	char *parent_remaning_path;
	char *remaning_path;
	char *dir_name;
	hret_t ret;

	ret = lookup_inode(path, &remaning_path, &to_remove);

	if(ret != RET_OK)
		return ret;

	ret = lookup_parent(path, &parent_remaning_path, &parent);
	dir_name = get_last_name(path);

	if(ret != RET_OK)
		return ret;

	// Si le répertoire parent n'a pas été trouvé
	if (parent_remaning_path != NULL)
	{
		kfree(parent_remaning_path);
		return -ERR_NOT_FOUND;
	}

	// Si le fichier n'a pas été trouvé
	if (remaning_path != NULL)
	{		
		kfree(parent_remaning_path);
		kfree(remaning_path);
		return -ERR_NOT_FOUND;
	}

	// Si l'inode est un répertoire
	if (to_remove->type != INODE_FILE)
		return -ERR_NOT_FILE;

	// Si l'inode est référencé plus d'une fois
	if (to_remove->memory_count > 1)
		return -ERR_BUSY;

	//Supprime le fichier du disque
	ret = parent->dir_op->unlink(parent, current, dir_name);
	unref_inode(to_remove);
	unref_inode(parent);
	return ret;
}
