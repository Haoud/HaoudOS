#include <lib/stdio.h>
#include <lib/string.h>
#include <lib/stdlib.h>
#include <core/mm/heap.h>
#include <core/fs/inode.h>
#include <core/syscall/syscall.h>

int sys_open(const char *path, uint32_t open_flags)
{
	struct inode *to_open = NULL;
	char *remaning_path = NULL;
	uint32_t fd_result = 0;	
	struct file *fd = NULL;
	int name_size = 0;
	char *name = NULL;
	hret_t ret;

	if ((open_flags & OPEN_DIRECTORY) && (open_flags & OPEN_CREAT))
		return -ERR_BAD_ARG;

repeat:
	ret = lookup_inode(path, &remaning_path, &to_open);

	if (ret != RET_OK)
		return ret;

	if (remaning_path != NULL)
	{
		// Ici, le chamin d'accès n'a pas été à 100 % résolu
		if (!(open_flags & OPEN_CREAT))
		{
			kfree(remaning_path);
			unref_inode(to_open);
			return -ERR_NOT_FOUND;
		}
		name_size = 0;

		while (remaning_path[name_size] != '\\' && remaning_path[name_size] != '\0')
			name_size++;

		// On alloue une chaine de caractères pour le nom
		name = kmalloc(name_size + 1);
		memcpy(name, remaning_path, name_size);
		name[name_size] = '\0';

		// Si l'entité à créer est un dossier
		while (remaning_path[name_size] == '\\' && remaning_path[name_size] != '\0')
			name_size++;

		// Si c'est un dossier qu'il faut créer
		if (remaning_path[name_size] != '\0')
		{
			// Seul mkdir peut crée un dossier
			kfree(name);
			kfree(remaning_path);
			unref_inode(to_open);
			return -ERR_NOT_FOUND;
		}

		// On créer le fichier
		ret = create_child_inode(current, to_open, name, INODE_FILE, 0);
		if (ret != RET_OK)
		{
			kfree(remaning_path);
			return ret;
		}

		unref_inode(to_open);
		kfree(remaning_path);
		clear_bits(open_flags, OPEN_CREAT);							// Change les options car le fichier vient d'être créé
		goto repeat;
	}
	else
	{
		// Ici, on peut directement accèder au fichier
		if ((open_flags & OPEN_CREAT) && (open_flags & OPEN_EXCL))
		{
			unref_inode(to_open);
			return -ERR_ALREADY_EXIST;
		}

		if (open_flags & OPEN_TRUNCATE)
		{
			if (to_open->inode_op->truncate != NULL)
			{
				ret = to_open->inode_op->truncate(to_open, 0);
				if (ret != RET_OK)
				{
					unref_inode(to_open);
					return ret;
				}
			}
		}
	}

	// Ici, on ouvre réellement le fichier
	ret = new_opened_file(current, to_open, open_flags, &fd);
	
	if (ret != RET_OK)
		return ret;
	
	// Et on insère le descripteur de fichier dans la liste
	register_open_file(current, fd, &fd_result);
	unref_inode(to_open);

	debugk("[OPEN] %u -> %s\n", fd_result, path);

	return fd_result;
}