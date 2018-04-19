#include <lib/stdio.h>
#include <core/mm/heap.h>
#include <core/fs/file.h>
#include <core/time/time.h>

/*
 * Augmente le compteur de référence d'un fichier ouvert
 */
hret_t ref_open_file(struct file *f)
{
	f->ref++;
	return RET_OK;
}

/*
 * Diminue le compteur de référence d'un fichier ouvert et le supprime de
 * la mémoire s'il devient inutile
 */
hret_t unref_open_file(struct file *f)
{
	hret_t ret;
	if (f->ref <= 0)
		panic("Tentative de libération d'un descripteur de fichier ouvert déjà libre !");

	f->ref--;

	if (f->ref == 0)
		goto delete_fd;
	else
		return RET_OK;

delete_fd:
	ret = f->inode->close_file(f->inode, f);		// On libère le descripteur de fichier
	unref_inode(f->inode);							// On indique que l'inode n'est plus utilisé
	return ret;										// Retourne le code de retour de "close_file"
}

/*
 * Cette fonction permet d'initialiser le tableau des fichiers ouvert (32 entrées)
 * d'un processus. Toutes les entrées sont initialisées à NULL
 */
hret_t init_fd_struct(struct process *p)
{
	for (int i = 0; i < MAX_OPENED_FILE; i++)
		p->open_files[i] = NULL;
	
	return RET_OK;
}

/*
 * Cette fonction permet de supprimer un fichier ouvert de la liste des fichiers ouvert d'un 
 * processus, mais ne le supprime pas de la mémoire (ce n'est pas à cette fonction de le faire)
 */
hret_t unregister_open_file(struct process *p, int fd_index)
{
	if (fd_index < 0 || fd_index >= MAX_OPENED_FILE)
		return -ERR_BAD_ARG;

	if (p->open_files[fd_index] == NULL)
		panic("Tentative de désenregistrement d'un descripteur de fichier inexistant");

	p->open_files[fd_index] = NULL;
	return RET_OK;
}

/*
 * Cette fonction permet de retourner une structure file à partir d'un processus et de l'index
 * du fichier ouvert dans le tableau open_file situé dans la structure du processus
 */
hret_t get_open_file(struct process *p, uint32_t fd_index, struct file **result)
{
	if (fd_index >= MAX_OPENED_FILE)
		return -ERR_BAD_ARG;

	(*result) = p->open_files[fd_index];
	return RET_OK;
}

/*
 * Cette fonction permet d'enregistrer un fichier ouvert dans le tableau des fichiers ouverts
 * d'un processus
 */
hret_t register_open_file(struct process *p, struct file *f, uint32_t *fd_index_result)
{
	hret_t ret;
	int fd_index;

	ret = found_free_process_fd(p, &fd_index);
	if (ret != RET_OK)								// Généralement c'est qu'il n'y a plus de place dans le tableau
		return ret;

	p->open_files[fd_index] = f;					// On enregistre le fichier
	if (fd_index_result)
		*fd_index_result = fd_index;				// On indique l'index où le fichier ouvert à été enregistré

	return RET_OK;
}

/*
 * Permet de trouver un emplacement de descripteur libre au sein d'un processus
 * Retourne un nombre indiquant l'index dans le tableau, sinon -1 et un code
 * d'erreur
 */
hret_t found_free_process_fd(struct process *p, int *result_index)
{
	for (int i = 0; i < MAX_OPENED_FILE; i++)
	{	
		if (p->open_files[i] == NULL)
		{
			*result_index = i;
			return RET_OK;
		}
	}

	*result_index = -1;
	return ERR_NOT_FOUND;
}

hret_t duplicate_open_file(struct file *fd, struct process *for_process, struct file **result)
{
	hret_t ret;

	ret = fd->duplicate(fd, for_process, result);

	if (ret != RET_OK)
		return ret;

	(*result)->ref = 1;
	(*result)->inode = fd->inode;
	(*result)->generation = 1;
	(*result)->open_flags = fd->open_flags;

	ref_inode(fd->inode);
	return RET_OK;
}

hret_t duplicate_fd_struct(struct process *modele, struct process *copieur)
{
	for (int i = 0; i < MAX_OPENED_FILE; i++)
	{
		if (modele->open_files[i] != NULL)
		{
			copieur->open_files[i] = modele->open_files[i];
			copieur->open_files[i]->ref++;
		}
	}

	return RET_OK;
}

/*
 * Cette fonction est appelé lorsqu'un processus ouvre un fichier: nous devons créer la structure
 * file à partir d'une structure inode
 */
hret_t new_opened_file(struct process *owner, struct inode *inode, flags_t open_flags, struct file **result)
{
	hret_t ret;

	ret = inode->open_file(inode, owner, open_flags, result);
	
	if (ret != RET_OK)
		return ret;

	(*result)->ref = 1;
	(*result)->inode = inode;
	(*result)->open_flags = open_flags;
	(*result)->generation = get_current_unix_time();	

	ref_inode(inode);			// Référence l'inode une fois de plus
	return RET_OK;
} 

/*
 * Obtient la taille en octet d'un fichier ouvert tout en conservant la
 * position actuelle du curseur intact
 */
size_t get_open_file_size(struct file *f)
{
	struct stat st;
	hret_t ret;

	ret = f->inode->inode_op->stat(f->inode, &st);
	
	if(ret != RET_OK)
		return 0;

	return (size_t)st.size;									// Retourne la taille du fichier en octet (MAX: 4GO)
}
