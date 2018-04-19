#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <lib/stdlib.h>
#include <i386/paging.h>
#include <core/mm/heap.h>
#include <core/mm/area.h>
#include <core/fs/dirent.h>
#include <core/fs/ramdisk/ramdisk.h>

static struct file_system ramdisk_fs;						// Syst�me de fichier RAMDISK
static struct super_operation ramdisk_super_op;				// Op�rations du super_block
static struct file_system_operation ramdisk_fs_op;			// Op�rations du syst�me de fichier

static struct inode_operation ramdisk_inode_op;				// Op�ration pour les fichiers
static struct inode_dir_operation ramdisk_dir_op;			// Op�rations pour les r�pertoires

static struct open_dir_operation ramdisk_open_dir_op;		// Op�ration pour un r�pertoire ouvert
static struct open_file_operation ramdisk_open_file_op;		// Op�ration pour un fichier ouvert

hret_t ramdisk_open_file(struct inode *this_inode, struct process *owner, flags_t open_flags, struct file **result);
hret_t ramdisk_link(struct inode *this_inode, const struct process *actor, const char *name, struct inode *inode_to_link);
hret_t ramdisk_duplicate_open_file(struct file *this_file, struct process *for_process, struct file **result);
hret_t ramdisk_allocate_inode(struct super_block *this_block, enum inode_type type, struct inode **result);
hret_t ramdisk_mount(struct file_system *this_fs, dev_t device, struct super_block **result_super);
hret_t ramdisk_fetch_inode(struct super_block *this_super, uint64_t inode_id, struct inode **result);
hret_t ramdisk_lookup(struct inode *this_inode, const char *name, uint64_t *result_inode_id);
hret_t ramdisk_unlink(struct inode *this_inode, struct process *actor, const char *name);
hret_t ramdisk_seek(struct file *this_file, seek_t whence, off_t offset, off_t *result);
hret_t ramdisk_umount(struct file_system *this_fs, struct super_block *to_umount);
hret_t ramdisk_resize_file(struct ramdisk_inode *to_resize, size_t new_size);
hret_t ramdisk_close_file(struct inode *this_inode, struct file *to_close);
hret_t ramdisk_write(struct file *this_file, char *src_buf, size_t *len);
hret_t ramdisk_read(struct file *this_file, char *dst_buf, size_t *len);
hret_t ramdisk_readdir(struct file *this_file, struct dirent *result);
hret_t ramdisk_truncate(struct inode *this_inode, off_t length);
hret_t ramdisk_inode_destructor(struct inode *to_del);
hret_t ramdisk_sync(struct inode *this_inode);

/*
 * Cette fonction est la fontion qui initialise le ramdisk et doit �tre appel�e apr�s l'initialisation
 * du VFS mais avant le montage du premier syst�me de fichier
 * C'est la seule fonction qui est appel� en dehors du VFS, les autres fonctions sont "cach�es".
 */
void RegisterRamdisk(void)
{
	ramdisk_fs_op.mount = ramdisk_mount;
	ramdisk_fs_op.umount = ramdisk_umount;

	ramdisk_fs.name = "ramdisk";
	ramdisk_fs.fs_op = &ramdisk_fs_op;
		
	ramdisk_super_op.allocate_inode = ramdisk_allocate_inode;	
	ramdisk_super_op.fetch_inode = ramdisk_fetch_inode;	

	ramdisk_dir_op.link = ramdisk_link;
	ramdisk_dir_op.unlink = ramdisk_unlink;
	ramdisk_dir_op.lookup = ramdisk_lookup;

	ramdisk_inode_op.sync = ramdisk_sync;
	ramdisk_inode_op.truncate = ramdisk_truncate;

	ramdisk_open_file_op.seek = ramdisk_seek;
	ramdisk_open_file_op.read = ramdisk_read;
	ramdisk_open_file_op.write = ramdisk_write;

	ramdisk_open_dir_op.readdir = ramdisk_readdir;

	register_fs(&ramdisk_fs);
}

/*
 * Cette fonction permet de redimensionner le contenu d'un fichier lorsque plus d'espace pour ce fichier
 * est n�cessaire. L'allocation des donn�es se fait par pages (4096 octets) pour plus d efficacit� et 
 * sont aussi align�es sur une page
 */
hret_t ramdisk_resize_file(struct ramdisk_inode *to_resize, size_t new_size)
{
	void *new_data_ptr = NULL;									// Pointeur sur les nouvelles
	uint32_t nb_pages_data = 0;									// Nombre de pages � allouer

	if (to_resize->file.allocated_size >= new_size)				// Si la nouvelle taille demand� � d�ja �t� pr�allou�e
	{
		to_resize->file.file_size = new_size;					// On change juste la taille r�elle du fichier
		return RET_OK;											
	}

	nb_pages_data = PHYS_ALIGN_SUP(new_size) / PAGE_SIZE;		// Calcul du nombre de pages n�cessaire
	new_data_ptr = (void *) get_vm_area(nb_pages_data, 0);		// Allocation de la nouvelle zone de m�moire
	PagingAutoMap((vaddr_t)new_data_ptr, nb_pages_data, VM_MAP_ATOMIC | VM_MAP_WRITE);			// Map la portion de m�moire

	memset(new_data_ptr, 0, nb_pages_data * PAGE_SIZE);			// On remplit la zone m�moire de 0

	if (to_resize->file.data_ptr != NULL)						// S'il existait d'autre donn�es avant
	{
		memcpy(new_data_ptr, to_resize->file.data_ptr, new_size);	// On recopie les donn�es
		PagingUnmap((vaddr_t)to_resize->file.data_ptr);
		FreeVmArea((vaddr_t)to_resize->file.data_ptr);				// On lib�re l'ancienne zone de m�moire si elle existe
	}

	// On actualise la taille du fichier
	to_resize->file.data_ptr = new_data_ptr;					// Pointeur vers les nouvelles donn�es
	to_resize->file.file_size = new_size;						// Taille du fichier 
	to_resize->file.allocated_size = nb_pages_data * PAGE_SIZE;	// Taille du fichier en m�moire

	return RET_OK;
}

hret_t ramdisk_read(struct file *this_file, char *dst_buf, size_t *len)
{
	struct ramdisk_inode *ram_inode = this_file->inode->private_data;

	if (this_file->inode->type != INODE_FILE)
		return -ERR_ACCES;

	if (this_file->offset >= ram_inode->file.file_size)
	{
		*len = 0;						// Rien � lire
		return RET_OK;					// Pas d'erreur
	}

	// Si la lecture entra�ne un d�bordement
	if (this_file->offset + *len >= ram_inode->file.file_size)
		*len = ram_inode->file.file_size - (size_t) this_file->offset;			// On s'assure qu'on ne lit pas en dehors du fichier

	// On copie les donn�es dans le buffer
	memcpy(dst_buf, (char *)(((uint32_t)ram_inode->file.data_ptr) + (size_t)this_file->offset), *len);

	//On actualise le position dans le fichier
	this_file->offset += *len;

	return RET_OK;
}

hret_t ramdisk_write(struct file *this_file, char *src_buf, size_t *len)
{
	struct ramdisk_inode *ram_inode = this_file->inode->private_data;

	if (this_file->inode->type != INODE_FILE)
		return -ERR_ACCES;

	/* Est ce qu'il faut redimmensionner le fichier ? */
	if ((this_file->offset + *len) >= ram_inode->file.file_size)
	{
		if (ramdisk_resize_file(ram_inode, ((size_t)this_file->offset) + *len) != RET_OK)
			*len = ram_inode->file.file_size - (size_t) this_file->offset;								// Si erreur alors on fait avec
	}

	// On copie les donn�es � partir du buffer
	memcpy( (char *)(((uint32_t)ram_inode->file.data_ptr) + (size_t)this_file->offset), src_buf, *len);

	//On actualise le position dans le fichier
	this_file->offset += *len;
	return RET_OK;
}

hret_t ramdisk_seek(struct file *this_file, seek_t whence, off_t offset, off_t *result)
{
	struct ramdisk_inode *ram_inode = this_file->inode->private_data;
	off_t starting_offset;

	if (this_file->inode->type != INODE_FILE)
		return -ERR_ACCES;

	switch (whence)
	{
		case SEEK_CUR:
			starting_offset = this_file->offset;
			break;

		case SEEK_END:
			starting_offset = ram_inode->file.file_size;
			break;

		case SEEK_SET:
			starting_offset = 0;
			break;

		default:
			return -ERR_BAD_ARG;
			break;
	}

	if (offset < -starting_offset)			// Si l'offset se positionne avant le d�but du fichier (le r�sultat sera n�gatif)
		return -ERR_BAD_ARG;				// On retourne une erreur

	this_file->offset = starting_offset + offset;
	*result = this_file->offset;
	return RET_OK;
}

hret_t ramdisk_readdir(struct file *this_file, struct dirent *result)
{
	/*
	 * Pour les r�pertoires, l'offset indique "l'ordre de creation" du dernier appel �
	 * "readdir", et la variable "custom_data" indique l'adresse de la derni�re entr�e 
	 * de r�pertoire lu
	 */
	struct ramdisk_dirent * direntry, *next_direntry;
	struct ramdisk_inode * ram_inode;
	int nb;

	ram_inode = this_file->inode->private_data;
	next_direntry = NULL;

	if ((this_file->generation == ram_inode->inode_super.generation) && (this_file->custom_data != NULL))
	{
		
		direntry = (struct ramdisk_dirent*)this_file->custom_data;		// Obtient l'entr�e de r�pertoire "courante"
		next_direntry = direntry->sibling_next;							// Obtient l'entr�e de r�pertoire suivante

		/* Somme nous � la fin de la liste des entr�es de r�pertoires ? */
		if (next_direntry == list_get_head(ram_inode->dir.entries))
			next_direntry = NULL;										// Si oui alors il n'y a pas de prochaine entr�e de r�pertoire
	}
	else
	{
		/*
		 * Ici, on doit rechercher la prochaine entr�e de r�pertoire dans 2 cas:
		 *		- Soit c'est le premier appel a readdir pour ce descripteur de fichier et il faut
		 *		  charger la premi�re entr�e de r�pertoire (normal)
		 *		- Soit entre deux appel a readdir, un entr�e de r�pertoire � �t� ajout� et ne doit
		 *		  pas �tre prise en compte
		 */
		next_direntry = NULL;
		// Recherche manuelle
		if (!list_empty(ram_inode->dir.entries))
		{
			list_foreach_named(ram_inode->dir.entries, direntry, nb, sibling_prev, sibling_next)
			{
				if (direntry->creat_order <= this_file->offset)
					continue;

				if (!next_direntry)
				{
					next_direntry = direntry;
					continue;
				}

				if (direntry->creat_order < next_direntry->creat_order)
					next_direntry = direntry;
			}
		}
	}

	// S'il ny a pas d'autre entr�e de r�pertoire
	if (!next_direntry)
	{
		this_file->custom_data = NULL;						// On indique qu'il ny a pas d'entr�e de r�pertoire actuelle
		this_file->offset = 0;								// Pas d'ordre de cr�ation non plus
		return -ERR_NO_ENTRY;								// On indique qu'il n'y a pas d'autres entr�es de r�pertoire
	}
		
	result->inode_id = ((vaddr_t)next_direntry->inode);					// Actualise l'identifiant de l'inode
	result->offset_in_dirfile = next_direntry->creat_order;				//
	result->type = next_direntry->inode->type;							// Indique le type d'inode
	memcpy(result->name, next_direntry->name, DIRENT_MAX_NAME_SIZE);	// Recopie le nom en m�moire

	this_file->offset = next_direntry->creat_order;						// Indique l'ordre de cr�ation de l'entr�e de r�pertoire
	this_file->custom_data = next_direntry;								// Indique la prochaine entr�e de r�pertoire si tout va bien

	return RET_OK;
}

hret_t ramdisk_duplicate_open_file(struct file *this_file, struct process *for_process, struct file **result)
{
	(*result) = kmalloc(sizeof(struct file));
	if (!(*result))
		return -ERR_NO_MEM;

	memcpy((char *)*result, (char *)this_file, sizeof(struct file));
	(*result)->owner = for_process;
	return RET_OK;;
}

hret_t ramdisk_sync(struct inode _unused *this_inode)
{
	// Pas besoin de synchroniser l'inodes et son contenu sur le disque car les inodes
	// sont d�j� stock�s en m�moire !!!
	return RET_OK;
}

hret_t ramdisk_open_file(struct inode *this_inode, struct process *owner, flags_t open_flags, struct file **result)
{
	struct file *fd = kmalloc(sizeof(struct file));

	if (!fd)
		return -ERR_NO_MEM;

	fd->offset = 0;
	fd->owner = owner;
	fd->custom_data = NULL;	
	fd->open_flags = open_flags;
	fd->duplicate = ramdisk_duplicate_open_file;

	fd->file_op = &ramdisk_open_file_op;			// Op�rations pour les fichiers
	if (this_inode->type == INODE_DIRECTORY)
		fd->dir_op = &ramdisk_open_dir_op;			// Op�rations sur les r�pertoires

	// Les p�riph�riques block et caract�res ne so,t pas g�r� par le syst�me de fichier

	(*result) = fd;
	return RET_OK;
}

hret_t ramdisk_close_file(struct inode _unused *this_inode, struct file *to_close)
{
	//On lib�re la m�moire et c'est tout !!!
	kfree(to_close);
	return RET_OK;
}

hret_t ramdisk_inode_destructor(struct inode *to_del)
{
	struct ramdisk_super *ram_super = to_del->super->custom_data;
	struct ramdisk_inode *ram_inode = to_del->private_data;

	if (to_del->disk_count == 0)
	{
		debugk("[RAMDISK]: Deleting inode in ramdisk...\n");				// Affiche un message de debug

		list_delete(ram_super->inode_list, ram_inode);						// Suprimme l'inode de la liste du super block
		kfree(ram_inode);													// Lib�re la m�moire utilis� par l'inode
	}

	return RET_OK;
}

hret_t ramdisk_truncate(struct inode *this_inode, off_t length)
{
	struct ramdisk_inode *ram_inode = this_inode->private_data;

	if (this_inode->type != INODE_FILE)
		return -ERR_ACCES;

	ramdisk_resize_file(ram_inode, (size_t)length);									// Rendimensionne le fichier
	return RET_OK;
}

/*
 * Cette fonction permet de r�cup�rer l'inode situ� dans le ramdisk. Cette fonction est tr�s simple car
 * l'inode est toujours en m�moire et inode_id est un pointeur convertit en une valeur 64 bits qui
 * pointe vers l'inode du ramdisk, qui lui, poss�de interi�urement un objet inode
 */
hret_t ramdisk_fetch_inode(struct super_block _unused *this_super, uint64_t inode_id, struct inode **result)
{
	struct ramdisk_inode *ram_inode;

	ram_inode = (struct ramdisk_inode *)((uint32_t)inode_id);
	(*result) = &ram_inode->inode_super;

	return RET_OK;
}

/*
 * Cette fonction permet d'allouer un inode sur le ramdisk et est relativement simple car les modifications
 * se font enti�rement en m�moire
 */
hret_t ramdisk_allocate_inode(struct super_block *this_block, enum inode_type type, struct inode **result)
{
	struct ramdisk_inode *ram_inode = NULL;	

	// On accepte seulement les fichier et les r�pertoires
	// Ou alors les p�riph�riques blocs ou caract�res
	if (type != INODE_FILE && type != INODE_DIRECTORY &&
		type != INODE_CHAR_DEVICE && type != INODE_BLOCK_DEVICE)
		return -ERR_UNKNOW;

	// On alloue l'inode sur le ramdisk
	ram_inode = kmalloc(sizeof(struct ramdisk_inode));

	// On met � jour le pointeur du pointeur de l'inode r�sulstant
	(*result) = &ram_inode->inode_super;

	// Et on modife ses variables
	(*result)->disk_count = 0;									// Nombre de r�f�rence � l'inode sur le disque (l'inode est cr�� mais pas r�f�renc�)
	(*result)->memory_count = 1;								// L'inode est pour l'instant r�f�renc� 1 fois
	(*result)->generation = 0;									// Pour �viter des probl�nes de d�synchronisation
	(*result)->inode_id = (uint64_t)((uint32_t)ram_inode);		// Pour pouvoir r�cup�rer l'inode � partir de son ID
	(*result)->is_dirty = FALSE;								// L'inode est modifi� (mais ne change pas grand chose ici)
	(*result)->private_data = ram_inode;						// Pointeur vers l'inode du ramdisk
	(*result)->super = this_block;								// super_block actuel 
	(*result)->type = type;										// Type de l'inode
	(*result)->inode_op = &ramdisk_inode_op;					// Op�rations pour tous les inodes


	// Les p�riph�riques bloc et caract�re sont g�r�s par le VFS (comme l'inode du RAMDISK = inode du VFS)
	if (type != INODE_CHAR_DEVICE && type != INODE_BLOCK_DEVICE)
	{
		if (type == INODE_FILE)
		{
			ram_inode->file.data_ptr = NULL;
			ram_inode->file.file_size = 0;
			ram_inode->file.allocated_size = 0;
		}
		else if (type == INODE_DIRECTORY)
		{
			list_init(ram_inode->dir.entries);						// On initialise la liste des ramdisk_dirent
			ram_inode->dir.top_creat_order = 0;						//
			(*result)->dir_op = &ramdisk_dir_op;					// Op�ration pour les r�pertoires
		}

		(*result)->open_file = ramdisk_open_file;
		(*result)->close_file = ramdisk_close_file;
	}

	(*result)->destructor = ramdisk_inode_destructor;			// Le destructeur est le m�me pour tout le monde (m�me pour les devices)

	// Ajoute l'inode dans la liste des indoes du ramdisk_super
	if (!list_empty(((struct ramdisk_super *)this_block->custom_data)->inode_list))
	{
		list_add_after(((struct ramdisk_super *)this_block->custom_data)->inode_list, ram_inode);
	}
	else
	{
		list_singleton(((struct ramdisk_super *)this_block->custom_data)->inode_list, ram_inode);
	}

	return RET_OK;
}

hret_t ramdisk_lookup(struct inode *this_inode, const char *name, uint64_t *result_inode_id)
{
	struct ramdisk_inode *ram_inode = this_inode->private_data;
	struct ramdisk_dirent *ram_cur_dirent;
	int nb_entry;

	if (!list_empty(ram_inode->dir.entries))
	{
		list_foreach_named(ram_inode->dir.entries, ram_cur_dirent, nb_entry, sibling_prev, sibling_next)
		{
			if (!strcmp(name, ram_cur_dirent->name))
			{
				(*result_inode_id) = ram_cur_dirent->inode->inode_id;
				return RET_OK;
			}
		}
	}
	
	(*result_inode_id) = 0;
	return -ERR_NO_ENTRY;
}

hret_t ramdisk_link(struct inode *this_inode, const struct process _unused *actor, const char *name, struct inode *inode_to_link)
{
	struct ramdisk_inode *parent = this_inode->private_data;
	struct ramdisk_dirent *ram_dirent;

	ram_dirent = kmalloc(sizeof(struct ramdisk_dirent));
	ram_dirent->name = kmalloc(strlen(name) + 1);

	memcpy(ram_dirent->name, name, strlen(name));
	ram_dirent->name[strlen(name)] = '\0';

	ram_dirent->inode = inode_to_link;
	this_inode->disk_count++;
	inode_to_link->disk_count++;

	if (list_empty(parent->dir.entries))
	{
		list_singleton_named(parent->dir.entries, ram_dirent, sibling_prev, sibling_next);
	}
	else
	{
		list_add_after_named(parent->dir.entries, ram_dirent, sibling_prev, sibling_next);
	}

	parent->dir.top_creat_order++;
	ram_dirent->creat_order = parent->dir.top_creat_order;

	return RET_OK;
}

hret_t ramdisk_unlink(struct inode *this_inode, struct process _unused *actor, const char *name)
{
	struct ramdisk_inode *ram_inode = this_inode->private_data;
	struct ramdisk_dirent *ram_cur_dirent;
	int nb_entry;

	if (!list_empty(ram_inode->dir.entries))
	{
		list_foreach_named(ram_inode->dir.entries, ram_cur_dirent, nb_entry, sibling_prev, sibling_next)
		{
			if (!strcmp(name, ram_cur_dirent->name))
			{
				list_delete_named(ram_inode->dir.entries, ram_cur_dirent, sibling_prev, sibling_next);

				ram_cur_dirent->inode->disk_count--;
				this_inode->disk_count--;

				kfree(ram_cur_dirent);
				return RET_OK;
			}
		}
	}

	return ERR_NO_ENTRY;
}

/*
 * Cette fonction permet de monter le syst�me de fichier ramdisk mais doit avant le cr�er car 
 * comme ce syst�me de fichier r�sident enti�rement en m�moire, son contenu est perdu en
 * int�gralit� et n'est pas sauvegard� sur un disque (bien qu'on le pourrait)
 */
hret_t ramdisk_mount(struct file_system *this_fs, dev_t _unused device, struct super_block **result_super)
{
	struct ramdisk_super *ramdisk_super;						// Contient un objet super_block
	struct inode *root;											// Inode qui va �tre allou�

	ramdisk_super = kmalloc(sizeof(struct ramdisk_super));		// Allocation du super block du ramdisk

	/* Initialisation du super_block et du ramdisk_super */
	memset((char *)ramdisk_super, 0, sizeof(struct ramdisk_super));		// Initialise le super block avec les valeurs par d�faut (0 ou NULL ...)

	ramdisk_super->super.fs = this_fs;							// Syst�me de fichier correspondant au super block (ramdisk)
	ramdisk_super->super.custom_data = ramdisk_super;			// Pour pouvoir acc�der au ramdisk_super depuis le super_block
	ramdisk_super->super.s_op = &ramdisk_super_op;				// Operation du super block

	list_init(ramdisk_super->inode_list);						// Initialisation de la liste des inodes du ramdisk_super
	list_init(ramdisk_super->super.dirty_list);					// Initialisation de la liste des inodes modifi�s
	list_init(ramdisk_super->super.used_list);					// Initialisation de la liste des inodes utilis�s

	/* Initialisation de l'inode racine */
	ramdisk_allocate_inode(&ramdisk_super->super, INODE_DIRECTORY, &root);					// Alloue l'inode et l'ajoute dans la ramdisk_super
	register_root_inode(&ramdisk_super->super, root);										// Enregistrer l'inode racine dans le super block

	if (result_super)
		*result_super = &ramdisk_super->super;

	return RET_OK;
}

/*
 * Cette fonction permet de d�monter le syst�me de fichier et de supprimer tous les fichiers
 * pr�sents sur ce syst�me de fichier car les modifications ne sont pas enregistr�es sur un
 * disque
 */
hret_t ramdisk_umount(struct file_system _unused *this_fs, struct super_block *to_umount)
{
	struct ramdisk_super *ram_super = to_umount->custom_data;			// Obtient le ramdisk_super associ�
	struct ramdisk_dirent *ram_dirent;									
	struct ramdisk_inode *ram_inode;

	/* Lib�ration des inodes en m�moire */
	while (!list_empty(ram_super->inode_list))
	{
		ram_inode = list_get_head(ram_super->inode_list);				// Obtient le premier inode de la liste (modifi� � chaque it�ration)
		
		if (ram_inode->inode_super.type == INODE_FILE)					// Si l'inode est un fichier
		{
			// On lib�re le contenu du fichier
			if (ram_inode->file.allocated_size)								// S'il il existe des donn�es
			{
				for(unsigned int i = 0; i < ram_inode->file.allocated_size; i += PAGE_SIZE)
					PagingUnmap((vaddr_t)ram_inode->file.data_ptr + i);

				FreeVmArea((vaddr_t)ram_inode->file.data_ptr);				// Lib�ration du contenu du fichier
			}
		}
		else if (ram_inode->inode_super.type == INODE_DIRECTORY)			// Si l'inode est un r�pertoire
		{
			// On lib�re tous les entr�es de r�pertoires (s'il y en a)
			while (!list_empty(ram_inode->dir.entries))
			{
				ram_dirent = list_get_head(ram_inode->dir.entries);		// Obtient la premi�re entr�e de r�pertoire (modifi� � chaque it�ration)

				kfree(ram_dirent->name);								// Lib�re le nom de la m�moire
				kfree(ram_dirent);										// Lib�re l'entr�e du r�pertoire de la m�moire
				list_delete_named(ram_inode->dir.entries, ram_dirent, sibling_prev, sibling_next);		// Supprime l'entr�e du r�pertoire de la liste
			}
		}

		// Dans tous les cas on lib�re l'inode
		list_delete(ram_super->inode_list, ram_inode);					// Enl�ve l'inode de la liste
		kfree(ram_inode);												// Lib�re l'inode de la m�moire
	}

	// On lib�re le ramdisk_super
	kfree(ram_super);
	return RET_OK;
}
