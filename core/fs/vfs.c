#include <lib/list.h>
#include <lib/ctype.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <lib/stdarg.h>
#include <lib/string.h>
#include <core/fs/vfs.h>
#include <core/mm/heap.h>
#include <lib/vsnprintf.h>
#include <core/fs/super.h>
#include <core/fs/inode.h>

/*
 * Cette fonction permet de préparer l'initialisation du système de fichier en 
 * initialisant certaines structures avec des valeurs par défaut
 */
void PrepareVFS(void)
{
	debugk("\n[VFS]: Starting initialisation...\n");
	init_mount_list();
}

#ifdef DEBUG_MODE
void vfs_debugk(const char *sys, const char *fmt, ...)
#else
void vfs_debugk(const char _unused *sys, const char _unused *fmt, ...)
#endif
{
#ifdef DEBUG_MODE
	char buffer[256];
	va_list arg;

	va_start(arg, fmt);
	vsnprintf(buffer, 256, fmt, arg);
	va_end(arg);

	debugk("[VFS %s]: %s\n", sys, buffer);
#endif
}


/*
 * Cette fonction permet d'initialiser le VFS avec le système de fichier de départ qui sera
 * monté sur la lettre de montage B
 */
void SetupVFS(const char *fs_name, const char *root_device, char mountpoint)
{
	struct file_system *start_fs = get_fs(fs_name);

	if (!start_fs)
		panic("Système de fichier %s non trouvé pour le système de fichier racine", fs_name);

	if(!is_letter(mountpoint))
		panic("Lettre de montage invalide pour initialiser le VFS");

	// On monte le système de fichier
	mount(mountpoint, fs_name, root_device);
}

void do_sync(void)
{
	sync_all_fs();
}

void sync_all_fs(void)
{
	struct super_block *current_super = NULL;

	for(int i = 0; i < MAX_MOUNT; i++)
	{
		current_super = get_super('A' + (char )i);

		if(current_super != NULL)
			sync_fs(current_super);
	}
}

/*
 * Cette fonction permet de synchroniser tous les inodes modifiés d'un super_block
 */
void sync_fs(struct super_block *to_sync)
{
	hret_t ret = RET_OK;

	while (list_get_head(to_sync->dirty_list) != NULL && ret == RET_OK)		// Pout tous les inodes modifié
		/*ret = */sync_inode(list_get_head(to_sync->dirty_list));			// On les réécrit sur le disque

	/*return ret;*/
}

hret_t parse_path(const char *path, char **parsed_path)
{
	unsigned int name_size;
	char *path_start;
	char *name_ptr;
	int path_len;
	char *name;

	if (!path)
		return -ERR_BAD_ARG;

	if (path[1] == ':')															// Si le chemin est absolu 
	{
		path_len = strlen(path) + 1;

		path_start = *parsed_path = kmalloc(path_len);
		memcpy(*parsed_path, path, path_len);
	} 
	else if(path[0] == '/')
	{
		path_len = strlen(path) + 1;
		path_start = *parsed_path = kmalloc(path_len + 3);

		memcpy(*parsed_path, "C:\\", 3);
		memcpy((*parsed_path) + 3, path + 1, path_len - 1);

	}
	else
	{
		path_len = strlen(current->cwd_path) + strlen(path) + 2;

		path_start = *parsed_path = kmalloc(path_len);

		memcpy(*parsed_path, current->cwd_path, strlen(current->cwd_path));
		if ((*parsed_path)[strlen(current->cwd_path) - 1] != '\\')
		{
			(*parsed_path)[strlen(current->cwd_path)] = '\\';								// On rajoute un \ entre les 2 deux chaînes (parfois facultatif)
			memcpy(*parsed_path + strlen(current->cwd_path) + 1, path, strlen(path) + 1);
		}
		else
		{
			memcpy(*parsed_path + strlen(current->cwd_path), path, strlen(path) + 1);
		}

	}

	// On remplace tous les / par des anti-/ pour la semi-compatibilité avec linux
	// Fonction pour toutes les applications qui n'essaye pas de monter un système
	// de fichier. En effet, elles tenteront de le monter dans C:, ce qui n'est pas*
	// possible est se traduira par une erreur
	strrpl(*parsed_path, '/', '\\'); 

	// On passe la lettre de lecteur ainsi que le :
	*parsed_path += 2;

	// On parse la chaine pour remplacer tout les . et ..
	while (**parsed_path != 0)
	{

		char *tmp_saved_parsed_path = *parsed_path;
		// On passe tous les \ de fin
		while (**parsed_path == '\\')
			(*parsed_path)++;

		// On ne garde que un '\'
		memcpy(tmp_saved_parsed_path + 1, *parsed_path, *parsed_path - tmp_saved_parsed_path);

		// Si c'est la fin de la chaîne de caractère
		if (**parsed_path == '\0')
			break;

		name_ptr = *parsed_path;
		name_size = 0;

		// On calcul la taille du nom de l'inode
		while (**parsed_path != '\\' && **parsed_path != '\0')
		{
			(*parsed_path)++;
			name_size++;
		}

		// On alloue une chaine de caractères pour le nom
		name = kmalloc(name_size + 1);
		memcpy(name, name_ptr, name_size);
		name[name_size] = '\0';

		if (strcmp(name, ".") == 0)
		{
			char *path_end = *parsed_path;

			while (**parsed_path == '\\')
				(*parsed_path)--;

			while (**parsed_path != '\\' && *parsed_path > path_start)
				(*parsed_path)--;

			memcpy(*parsed_path, path_end, path_len - (*parsed_path - path_start));
		}
		else if (strcmp(name, "..") == 0)
		{
			char *path_end = *parsed_path;

			while (**parsed_path == '\\')
				(*parsed_path)--;

			while (**parsed_path != '\\' && *parsed_path > path_start)
				(*parsed_path)--;

			if (*parsed_path <= path_start)
			{
				*parsed_path = path_end - 2;
				goto copy_and_retry;
			}

			while (**parsed_path == '\\')
				(*parsed_path)--;

			if (*parsed_path <= path_start)
			{
				*parsed_path = path_end - 2;
				goto copy_and_retry;
			}

			while (**parsed_path != '\\' && *parsed_path > path_start)
				(*parsed_path)--;

			if (*parsed_path <= path_start)
			{
				(*parsed_path) = path_end - 2;
				goto copy_and_retry;
			}

			memcpy(*parsed_path, path_end, path_len - (*parsed_path - path_start));
			continue;
			
		copy_and_retry:
			memcpy(*parsed_path, path_end, path_len - (*parsed_path - path_start));
			(*parsed_path) = path_start;
		}
	}

	*parsed_path = path_start;

	// Pour éviter un chemin d'accès comme C: (Il faut toujours le \\)
	if(*(path_start + 1) == ':' && *(path_start + 2) != '\\')
	{
		*(path_start + 2) = '\\';
		*(path_start + 3) = '\0';		
	}

	return RET_OK;
}

/*
 * Cette fonction prend en argument un chemin d'accès à un fichier et retourne
 * le nom de ce fichier
 *
 * Exemple: Si path = "C:\test\test.txt", cette fonction renverra "test.txt"
 *
 * NOTE: Le nom renvoyé est juste un pointeur sur la chaine de caractères
 * passée en argument, il ne faut pas la supprimer tant que l'on utilise
 * le nom retourné par cette fonction
 */
char *get_last_name(const char *path)
{
	char *path_ptr;

	path_ptr = (char *)(path + strlen(path));			// On se place à la fin du path

	// On se place au niveau du dernier répertoire
	while (*path_ptr != '\\' && path_ptr != path)
		path_ptr--;

	return ++path_ptr;									// On renvoye le nom sans le \ de fin
}

/*
 * Les fontion ci dessous ne doivent être qu'utilisé au démarrage du système pour
 * éviter des interblocages: Elle servent notamment à inclure les fichiers 
 * indispensable au fonctionnement d'HaoudOS si le système de fichier est en mémoire
 * (ramdisk fs). C'est le cas car HaoudOS ne supporte pas encore les périphériques
 * blocs ainsi que d'autres système de fichier
 */

/*
 * Cete fonction permet au noyau de créer un nouveau fichier à l'emplacement
 * spécifié en paramètre (pathname)
 */
hret_t kernel_create_file(const char *pathname)
{
	hret_t ret;
	char *filename;
	char *remaning_path;

	struct inode *parent;

	ret = lookup_parent(pathname, &remaning_path, &parent);

	if (ret != RET_OK)
		return ret;

	if (remaning_path != NULL)
	{
		unref_inode(parent);
		return ERR_NOT_FOUND;
	}

	filename = get_last_name(pathname);
	ret = create_child_inode(&process_list[0], parent, filename, INODE_FILE, 0);

	if (ret != RET_OK)
	{
		unref_inode(parent);
		return ret;
		
	}

	unref_inode(parent);

	return RET_OK;
}

/*
 * Cete fonction permet au noyau d'écrire dans le fichier à l'emplacement
 * spécifié en paramètre (pathname)
 */
hret_t kernel_write_file(const char *pathname, const char *buffer, off_t offset, size_t len)
{
	hret_t ret;							// Code de retour
	size_t saved_len;
	off_t result_offset;
	char *remaning_path;

	struct inode *i_file;				// Inode du fichier correspondant à pathname
	struct file *open_file;				// Fichier ouvert de l'inode correspondant à pathname

	saved_len = len;

	// On recherche l'inode du fichier à écrire
	ret = lookup_inode(pathname, &remaning_path, &i_file);

	if (ret != RET_OK)
		return ret;

	// Si le chemin d'accès n'a pas été résolu à 100 %
	if (remaning_path != NULL)
	{
		kfree(remaning_path);
		unref_inode(i_file);
		return -ERR_NOT_FOUND;
	}

	// On vérifie que l'inode est bien un fichier
	if (i_file->type != INODE_FILE)
	{
		unref_inode(i_file);
		return -ERR_NOT_FILE;
	}

	// On ouvre le fichier
	ret = new_opened_file(&process_list[0], i_file, OPEN_WRITE, &open_file);			// Fichier ouvert au nom du processus idle
	if (ret != RET_OK)
	{
		unref_inode(i_file);
		return ret;
	}

	// On se place au bon endroits
	open_file->file_op->seek(open_file, SEEK_SET, offset, &result_offset);
	// On écrit dans le fichier
	open_file->file_op->write(open_file, (char *)buffer, &len);	

	if (len != saved_len)
	{
		unref_open_file(open_file);
		unref_inode(i_file);

		return -ERR_UNKNOW;
	}

	// On ferme le fichier car les données ont été lues
	unref_open_file(open_file);
	return RET_OK;
}
