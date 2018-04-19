#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <core/fs/vfs.h>
#include <core/mm/heap.h>
#include <core/fs/inode.h>
#include <core/fs/file_system.h>

static struct file_system *fs_list = NULL;

/*
 * Cette fonction permet à un systeme de fichier de se désenregistrer du
 * VFS, mais seulement si ce dernièr n'est plus utilisé
 */
void unregister_fs(const char *fs_name)
{
	struct file_system *cur_fs;
	uint32_t nb_fs;

	if (!list_empty(fs_list))
	{
		list_foreach(fs_list, cur_fs, nb_fs)
		{
			if (!strcmp(fs_name, cur_fs->name))
			{
				if (cur_fs->super_list != NULL)
				{
					debugk("[VFS]: ERROR: Trying to free used file system structure !\n");
					return;
				}

				list_delete(fs_list, cur_fs)
				return;
			}
		}

		debugk("[VFS]: Unable to delete %s: not found in file system list!\n", fs_name);
		return;
	}

	debugk("[VFS]: Unable to delete %s: file system list is empty!\n", fs_name);
	return;
}

/*
 * Cette fonction permet à un système de fichier d'être pris en charge par le
 * VFS: tout les sytèmes de fichiers utilisé doivent s'enregistrer par cette
 * fonction
 */
void register_fs(struct file_system *fs)
{
	// On actualise certaines variables réservées
	list_init(fs->super_list);

	if (list_empty(fs_list))
	{
		list_singleton(fs_list, fs);
	}
	else
	{
		list_add_after(fs_list, fs);
	}

	debugk("[VFS]: Adding %s to file system list\n", fs->name);
}

/*
 * Cette fonction retourne unee structure file_system à partir d'un nom de
 * système de fichier
 */
struct file_system *get_fs(const char *name)
{
	struct file_system *cur_fs;
	int nb_fs;

	list_foreach(fs_list, cur_fs, nb_fs)
	{
		if (!strcmp(name, cur_fs->name))
			return cur_fs;
	}

	return NULL;					// Aucun systme cde fichier correspondant
}

/*
 * Cette fonction permet de démonter un système de fichier mais ne prend pas en compte les
 * différentes autorisations, c'est à la fonction appellante de la faire
 */
hret_t umount(char drive)
{
	struct super_block *to_umount;
	struct inode *inode;
	int nb_inodes;

	to_umount = get_super(drive);
	if (!to_umount)														// Si l'entrée n'a pas été trouvé
		return -ERR_NOT_FOUND;											// On quitte en signalant une erreur

	if(list_empty(to_umount))											// Si l'entrée semble corrompu
		return -ERR_NOT_FOUND;											// On quitte en signalant une erreur

	list_foreach_named(to_umount->used_list, inode, nb_inodes, used_prev, used_next)
	{
		if(nb_inodes > 2)
			break;
	}
	
	// Si le système de fichier est utilisé (sans compter l'inode root)
	if(nb_inodes > 1)
		return -ERR_BUSY;

	// Cas étrange (l'inode root n'est pas utilisé mais d'autres inodes le sont)
	if(list_get_head(to_umount->used_list) != to_umount->root)
		return -ERR_BUSY;												
	
	unref_inode(to_umount->root);										// Déréférence l'inode root
	sync_fs(to_umount);													// Synchronise le système de fichier
	to_umount->fs->fs_op->umount(to_umount->fs, to_umount);				// Démonte le système de fichier
	remove_super(drive);												// Supprime le super block de la liste de montage	

	return RET_OK;
}

hret_t mount(char drive, const char *fs_name, const char *device)
{
	struct file_system *concerned_fs;									// Système de fichier définit par fs_name
	struct inode *device_inode;											// Inode se référencant a un périphérique
	struct super_block *super;											// Super block qui va être crée
	char *remaning = NULL;
	dev_t device_id;
	hret_t ret_val;

	concerned_fs = get_fs(fs_name);

	if (!concerned_fs)
		return -ERR_BUSY;

	if(device != NULL)
	{
		ret_val = lookup_inode(device, &remaning, &device_inode);
		if(ret_val != RET_OK)
			return ret_val;

		if(remaning != NULL)
		{
			vfs_debugk("MOUNT", "Unable to load device inode");
			kfree(remaning);
			unref_inode(device_inode);
			return -ERR_NO_DEV;
		}
		
		device_id = device_inode->dev;
		unref_inode(device_inode);
	}
	else
	{
		device_inode = NULL;
		device_id = 0;
	}
	
	ret_val = concerned_fs->fs_op->mount(concerned_fs, device_id, &super);			// Monte le système de fichier
	debugk("[VFS]: Root inode id is %u\n", (uint32_t)super->root->inode_id);

	if (ret_val < 0)
		return ret_val;

	add_super(super, drive);											// Ajoute le super block à la liste de montage
	return RET_OK;
}
