#include <lib/stdio.h>
#include <core/fs/vfs.h>
#include <core/mm/heap.h>
#include <core/fs/inode.h>
#include <core/syscall/syscall.h>

int sys_rmdir(const char *pathname)
{
	struct inode *to_remove;
	struct inode *parent;
	char *parent_remaning_path;
	char *remaning_path;
	char *dir_name;
	hret_t ret;

	ret = lookup_inode(pathname, &remaning_path, &to_remove);
	ret = lookup_parent(pathname, &parent_remaning_path, &parent);
	dir_name = get_last_name(pathname);

	// Si le r�pertoire parent n'a pas �t� trouv�
	if (parent_remaning_path != NULL)
	{
		kfree(parent_remaning_path);
		return -ERR_NOT_FOUND;
	}

	// Si le r�pertoire n'a pas �t� trouv�
	if (remaning_path != NULL)
	{
		kfree(parent_remaning_path);
		kfree(remaning_path);
		return -ERR_NOT_FOUND;
	}

	// Si l'inode n'est pas un r�pertoire
	if (to_remove->type != INODE_DIRECTORY)
		return -ERR_NOT_DIR;

	// Si l'inode est r�f�renc� plus d'une fois
	if (to_remove->memory_count > 1)
		return -ERR_BUSY;

	// Si le r�pertoire n'est pas vide
	if (to_remove->disk_count > 1)
		return -ERR_NOT_EMPTY;

	//Supprime le r�pertoire du disque
	ret = parent->dir_op->unlink(parent, current, dir_name);
	unref_inode(to_remove);
	unref_inode(parent);
	return ret;
}
