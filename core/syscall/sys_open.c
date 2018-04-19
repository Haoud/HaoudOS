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
		// Ici, le chamin d'acc�s n'a pas �t� � 100 % r�solu
		if (!(open_flags & OPEN_CREAT))
		{
			kfree(remaning_path);
			unref_inode(to_open);
			return -ERR_NOT_FOUND;
		}
		name_size = 0;

		while (remaning_path[name_size] != '\\' && remaning_path[name_size] != '\0')
			name_size++;

		// On alloue une chaine de caract�res pour le nom
		name = kmalloc(name_size + 1);
		memcpy(name, remaning_path, name_size);
		name[name_size] = '\0';

		// Si l'entit� � cr�er est un dossier
		while (remaning_path[name_size] == '\\' && remaning_path[name_size] != '\0')
			name_size++;

		// Si c'est un dossier qu'il faut cr�er
		if (remaning_path[name_size] != '\0')
		{
			// Seul mkdir peut cr�e un dossier
			kfree(name);
			kfree(remaning_path);
			unref_inode(to_open);
			return -ERR_NOT_FOUND;
		}

		// On cr�er le fichier
		ret = create_child_inode(current, to_open, name, INODE_FILE, 0);
		if (ret != RET_OK)
		{
			kfree(remaning_path);
			return ret;
		}

		unref_inode(to_open);
		kfree(remaning_path);
		clear_bits(open_flags, OPEN_CREAT);							// Change les options car le fichier vient d'�tre cr��
		goto repeat;
	}
	else
	{
		// Ici, on peut directement acc�der au fichier
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

	// Ici, on ouvre r�ellement le fichier
	ret = new_opened_file(current, to_open, open_flags, &fd);
	
	if (ret != RET_OK)
		return ret;
	
	// Et on ins�re le descripteur de fichier dans la liste
	register_open_file(current, fd, &fd_result);
	unref_inode(to_open);

	debugk("[OPEN] %u -> %s\n", fd_result, path);

	return fd_result;
}