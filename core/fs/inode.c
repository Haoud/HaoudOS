#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <lib/string.h>
#include <core/fs/vfs.h>
#include <core/mm/heap.h>
#include <core/fs/inode.h>
#include <core/dev/char/char.h>
#include <core/dev/block/block.h>
#include <core/process/process.h>

void ref_inode(struct inode *inode)
{
	//debugk("[VFS]: Referecing inode id %u, count %u\n", (uint32_t)inode->inode_id, inode->memory_count);
	inode->memory_count++;
}

void unref_inode(struct inode *inode)
{
	//debugk("[VFS]: Unreferecing inode id %u, count %u\n", (uint32_t)inode->inode_id, inode->memory_count);
	if (inode->memory_count == 1)
		goto delete_inode;
	else if (inode->memory_count < 1)
		panic("Libération d'un i-node déjà libre");

	inode->memory_count--;
	return;

delete_inode:
	//debugk("[VFS]: Deleting inode id %u\n", (uint32_t)inode->inode_id);
	// Ici, l'inode va être supprimé de la mémoire
	if (inode->is_dirty)
		sync_inode(inode);															// Si l'inode est modifié alors on l'écrit sur le disque

	inode->memory_count--;

	if(inode->type != INODE_PIPE)
		{list_delete_named(inode->super->used_list, inode, used_prev, used_next);}	// Enlève l'inode de la liste des inode utilisés

	inode->destructor(inode);														// Détruit l'inode de la mémoire (sauf exceptions)
	inode = NULL;																	// Par sécurité
	return;
}

/*
 * Cette fonction permet de synchroniser un inode sur le disque
 */
void sync_inode(struct inode *to_sync)
{
	hret_t retval;

	vfs_debugk("SYNC", "Syncing diry inode %u", to_sync->inode_id);

	if (!to_sync->is_dirty)															// Si l'inode n'est pas sale alors pas besoin de le réécrire
		return;

	retval = to_sync->inode_op->sync(to_sync);										// Synchronise l'inode sur le disque
	if(retval != RET_OK)
		return;

	to_sync->is_dirty = FALSE;														// Indique que l'inode n'est plus modifié
	list_delete_named(to_sync->super->dirty_list, to_sync, dirty_prev, dirty_next);	// Enlève l'inode de la liste des inode utilisés
}

/*
 * Cette fonction permet de marquer un inode comme étant modifié et l'ajoute à la liste
 * des inodes modifiés s'il ny était pas avant l'appel à cette fonction
 */
void mark_inode_as_dirty(struct inode *to_mark)
{
	bool_t was_dirty = to_mark->is_dirty;

	to_mark->is_dirty = TRUE;
	to_mark->generation++;

	vfs_debugk("MARK INODE", "Marking inode %u dirty", to_mark->inode_id);

	if (!was_dirty && to_mark->is_dirty)											// Uniquement si l'inode vient d'être modifié
	{
		vfs_debugk("MARK INODE", "Adding inode %u in dirty list", to_mark->inode_id);
		if (list_empty(to_mark->super->dirty_list))
			list_singleton_named(to_mark->super->dirty_list, to_mark, dirty_prev, dirty_next);
		else
			list_add_after_named(to_mark->super->dirty_list, to_mark, dirty_prev, dirty_next);
		
	}

	if (to_mark->super->flags & SUPER_MOUNT_SYNC)									// Si le super block nécéssite une réécriture directe
		sync_inode(to_mark);														// Synchronise l'inode sur le disque
}

void register_root_inode(struct super_block *super, struct inode *root)
{
	super->root = root;
	list_singleton_named(super->used_list, root, used_prev, used_next);
}

/*
 * Cette fonction permet de rechercher un inode par son chemin d'accès (comme "C:\HaoudOS\Shell.exe")
 * qui est un pointeur vers une chaine de caractère (path) et returne le dernière inode auquel cette
 * fonction a pu avoir accès. Si par exemple, si le dossier HaoudOS n'existerai pas, alors cette  
 * fonction reverait l'inode correspondant à la racine (C:) et la varaible de sortie remaning_path
 * contiendrai la pârtir qui ne pas put être recherchée (ici, "HaoudOS\Shell.exe")
 * 
 * NOTE IMPORTANTE: Si remaning_path n'esy pas nul lors ce que la fonction retourne, la fonction appelante
 * doit penser à liberer la mémoire avec un kfree sur le remaning path
 */
hret_t lookup_inode(const char *_path, char **remaning_path, struct inode **result)
{
	struct inode *current_inode;
	struct inode *start_inode;
	struct super_block *super;	
	struct inode *next_inode;
	unsigned int name_size;
	unsigned total_len = 0;
	uint64_t tmp_inode_id;
	char *path_start;
	char *name_ptr;
	hret_t retval;
	bool_t exit;
	char *path;
	char *name;

	(*remaning_path) = NULL;
	(*result) = NULL;
	exit = FALSE;

	if (!_path)
		return -ERR_BAD_ARG;

	retval = parse_path(_path, &path);

	if (retval != RET_OK)
		return retval;

	super = get_super(path[0]);
	
	if(super == NULL)
	{
		(*remaning_path) = (char *)_path;
		kfree(path);
		return -ERR_ACCES;
	}

	start_inode = super->root;
	current_inode = start_inode;
	path_start = path;
	path += 2;

	while (!exit)
	{
		ref_inode(current_inode);

	search:
		// On passe tous les \ de fin
		while (*path == '\\')
			path++;
		
		// Si c'est la fin de la chaîne de caractère
		if (*path == '\0')
		{
			(*result) = current_inode;												
			(*remaning_path) = NULL;												// On indique qu'il n'y a pas de chemin restant
			kfree(path_start);

			return RET_OK;															// On quitte car on a trouvé l'inode
		}

		// On vérifie si l'inode actuel est un répertoire
		if (current_inode->type != INODE_DIRECTORY)									// Si l'inode n'est pas un répertoire
		{
			unref_inode(current_inode);
			(*remaning_path) = NULL;
			kfree(path_start);
			return -ERR_NO_DIR;														// On quitte car on ne peut plus rechercher les autres inodes
		}

		name_ptr = path;
		name_size = 0;

		// On calcul la taille du nom de l'inode
		while (*path != '\\' && *path != '\0')
		{
			path++;
			name_size++;
		}

		total_len += name_size;

		// On alloue une chaine de caractères pour le nom
		name = kmalloc(name_size + 1);
		memcpy(name, name_ptr, name_size);
		name[name_size] = '\0';

		if (strcmp(name, ".") == 0)
		{
			goto search;		// N'est normalement jamais atteind
		}
		else if (strcmp(name, "..") == 0)
		{
			goto search;		// Si certains .. n'ont pas pu être résolu, on les ignore, car on ne peut pas remonter plus loin que la racine
		}

		// Ici, on essaye de trouver l'inode dans le répetoire
		retval = current_inode->dir_op->lookup(current_inode, name, &tmp_inode_id);
		
		if (retval != RET_OK)
		{
			// Ici, on ne peut pas aller plus loin
			(*result) = current_inode;
			(*remaning_path) = kmalloc(strlen(name_ptr) + 1);
			strcpy(*remaning_path, name_ptr);

			kfree(name);
			kfree(path_start);

			return RET_OK;
		}

		//Ici on récupère l'inode
		retval = fetch_inode(current_inode->super, tmp_inode_id, &next_inode);

		if (retval != RET_OK)
		{
			// Ici, erreur !!!
			(*remaning_path) = kmalloc(strlen(path) - total_len + 1);
			strcpy(*remaning_path, path + total_len);

			unref_inode(current_inode);
			kfree(name);
			kfree(path_start);

			return retval;
		}

		unref_inode(current_inode);
		current_inode = next_inode;
		kfree(name);
	}

	kfree(path_start);
	unref_inode(current_inode);
	return -ERR_UNKNOW;
}

hret_t lookup_parent(const char *_path, char **remaning_path, struct inode **result)
{
	char *parent_path;
	char *path_ptr;
	hret_t ret;

	path_ptr = (char *)(_path + strlen(_path));			// On se place à la fin du path

	// On se place au niveau du dernier répertoire
	while (*path_ptr != '\\' && path_ptr != _path)
		path_ptr--;

	if (path_ptr == _path)								// Si on essaye de chercher le parent de la racine
		return -ERR_ACCES;

	// On alloue une chaine de caractères pour le nopath
	parent_path = kmalloc((path_ptr - _path) + 1);
	memcpy(parent_path, _path, (path_ptr - _path));
	parent_path[(path_ptr - _path)] = '\0';				// Caractère nul de fin de chaîne

	ret = lookup_inode(parent_path, remaning_path, result);

	if (ret != RET_OK)
	{
		kfree(parent_path);
		return ret;
	}

	if ((*result)->type != INODE_DIRECTORY)
	{
		kfree(parent_path);
		unref_inode((*result));
		return -ERR_NOT_DIR;
	}

	return RET_OK;
}

/**/
hret_t fetch_inode(struct super_block *super, uint64_t inode_id, struct inode **result)
{
	struct inode *inode;
	hret_t retval;
	int nb_inode;
	bool_t is_empty = FALSE;

	// Cherche si l'inode est déjà en mémoire
	if (!list_empty(super->used_list))
	{
		list_foreach_named(super->used_list, inode, nb_inode, used_prev, used_next)
		{
			if (inode->inode_id == inode_id)
			{
				(*result) = inode;
				return RET_OK;
			}
		}
	}
	else
	{
		is_empty = TRUE;
	}
	
	// Si l'inode n'est pas en mémoire, on le charge en mémoire
	retval = super->s_op->fetch_inode(super, inode_id, result);

	if (retval != RET_OK)
		return retval;

	(*result)->generation = 0;																// Indique que l'inode ...

	if ((*result)->type == INODE_CHAR_DEVICE)												// Si l'inode est un périphérique caractère
		chardev_ref_new_inode(*result);														// Un traitement spécial est requis
	if ((*result)->type == INODE_BLOCK_DEVICE)												// Si l'inode est un périphérique block
		blockdev_ref_new_inode(*result);													// Un traitement spécial est requis
		
	if (!is_empty)
		list_add_after_named(super->used_list, (*result), used_prev, used_next);			// Ajoute l'inode dans la liste des inodes utilisés
	else
		list_singleton_named(super->used_list, (*result), used_prev, used_next);			// Ajoute l'inode dans la liste des inodes utilisés
	

	return RET_OK;
}

hret_t allocate_inode(struct super_block *super, enum inode_type type, struct inode **result)
{
	hret_t retval;

	if (super->flags & SUPER_MOUNT_READ_ONLY)
		return -ERR_PERM;

	retval = super->s_op->allocate_inode(super, type, result);

	if (retval != RET_OK)
		return retval;

	(*result)->super = super;															// Actualise quelques variables réservées
	
	if (!list_empty(super->used_list))
		list_add_after_named(super->used_list, (*result), used_prev, used_next);		// Ajoute l'inode dans la liste des inodes utilisés
	else
		list_singleton_named(super->used_list, (*result), used_prev, used_next);		// Ajoute l'inode dans la liste des inodes utilisés

	mark_inode_as_dirty(*result);														// Marque l'inode comme étant modifié car nouvellement crée
 	return RET_OK;
}

hret_t create_inode(const char *_path, const struct process *actor, enum inode_type type)
{
	hret_t ret;
	char *name;
	size_t name_size = 0;
	char *remaning_path;
	struct inode *parent;

	ret = lookup_inode(_path, &remaning_path, &parent);

	if (ret != RET_OK)
		return ret;

	// Si l'inode existe déjà
	if (remaning_path == NULL)
	{
		unref_inode(parent);
		return -ERR_ALREADY_EXIST;
	}

	// On calcul la taille du nom de l'inode
	while (*remaning_path != '\\' && *remaning_path != '\0')
	{
		remaning_path++;
		name_size++;
	}

	name = kmalloc(name_size + 1);
	memcpy(name, (remaning_path - name_size), name_size);
	name[name_size] = '\0';

	ret = create_child_inode(actor, parent, name, type, 0);
	unref_inode(parent);
	kfree(remaning_path);
	kfree(name);
	return ret;
}

hret_t make_speacial_node(const char *_path, const struct process *actor, enum inode_type type, dev_t devid)
{
	hret_t ret;
	char *remaning_path = NULL;
	struct inode *created_inode;

	if (type != INODE_CHAR_DEVICE && type != INODE_BLOCK_DEVICE)
		panic("make_speacial_node(): trying to creating none-device node");

	ret = create_inode(_path, actor, type);
	if (ret != RET_OK)
		return ret;

	ret = lookup_inode(_path, &remaning_path, &created_inode);
	if (ret != RET_OK)
		return ret;

	if (remaning_path != NULL)
	{
		kfree(remaning_path);
		return -ERR_ACCES;
	}

	if (created_inode == NULL)
		return -ERR_UNKNOW;

	created_inode->dev = devid;
	mark_inode_as_dirty(created_inode);			// TODO: Verifier si juste
	unref_inode(created_inode);
	return ret;
}

hret_t create_child_inode(const struct process *actor, struct inode *parent, const char *name, enum inode_type type, uint32_t flags)
{
	hret_t ret;
	struct inode *new_inode;

	// Si l'inode parent n'est pas un répertoire
	if (parent->type != INODE_DIRECTORY)
		return -ERR_NO_DIR;

	// Si le nom n'est pas valide
	if (!name)
		return -ERR_BAD_ARG;

	// Si le nom commence par \0
	if (*name == '\0')
		return -ERR_BAD_ARG;

	// On crée l'inode
	ret = allocate_inode(parent->super, type, &new_inode);

	if (ret != RET_OK)
		return ret;

	//On enreigstre l'inode dans l'inode parent
	register_child_inode(actor, parent, name, new_inode, flags);
	unref_inode(new_inode);
	return RET_OK;
}

hret_t register_child_inode(const struct process *actor, struct inode *parent, const char *name, struct inode *to_register, uint32_t _unused flags)
{
	hret_t ret;


	// Si l'inode parent n'est pas un répertoire
	if (parent->type != INODE_DIRECTORY)
		return -ERR_NO_DIR;

	// Si le nom n'est pas valide
	if (!name)
		return -ERR_BAD_ARG;

	// Si le nom commence par \0
	if (*name == '\0')
		return -ERR_BAD_ARG;

	// Si les deux inodes ne sont pas sur le même point de montage
	if (parent->super != to_register->super)
		return -ERR_ACCES;

	ret = parent->dir_op->link(parent, actor, name, to_register);		// On enregistre l'inode dans le répertoire

	if (ret != RET_OK)
		return ret;

	mark_inode_as_dirty(parent);										// On considère que l'inode est sale
	mark_inode_as_dirty(to_register);									// On considère que l'inode est sale

	return RET_OK;
}

hret_t connext_existing_child_inode(const struct process *actor, struct inode *parent, const char *name, struct inode *to_register)
{
	hret_t ret;

	// Si l'inode parent n'est pas un répertoire
	if (parent->type != INODE_DIRECTORY)
		return -ERR_NO_DIR;

	// Si le nom n'est pas valide
	if (!name)
		return -ERR_BAD_ARG;

	// Si le nom commence par \0
	if (*name == '\0')
		return -ERR_BAD_ARG;

	// Si les deux inodes ne sont pas sur le même point de montage
	if (parent->super != to_register->super)
		return -ERR_ACCES;

	ret = parent->dir_op->link(parent, actor, name, to_register);		// On enregistre l'inode dans le répertoire

	if (ret != RET_OK)
		return ret;

	mark_inode_as_dirty(parent);										// On considère que l'inode est sale
	mark_inode_as_dirty(to_register);									// On considère que l'inode est sale

	return RET_OK;
}
