#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/mm/heap.h>
#include <core/fs/ext2/ext2.h>
#include <core/dev/block/block.h>

hret_t ext2_open_file(struct inode *this_inode, struct process *owner, flags_t open_flags, struct file **result)
{
    struct file *fd = kmalloc(sizeof(struct file));
    
    if (!fd)
        return -ERR_NO_MEM;

    fd->offset = 0;
    fd->owner = owner;
    fd->custom_data = NULL;	
    fd->open_flags = open_flags;
    fd->duplicate = ext2_duplicate_open_file;

    fd->file_op = &ext2_open_file_op;			// Opérations pour les fichiers
    if (this_inode->type == INODE_DIRECTORY)
        fd->dir_op = &ext2_open_dir_op;			// Opérations sur les répertoires

    // Les périphériques block et caractères ne sont pas géré par le système de fichier
    // Mais par le VFS, même s'ils sont stockés sur le disque dur
    (*result) = fd;
    return RET_OK;
}

hret_t ext2_duplicate_open_file(struct file *this_file, struct process *for_process, struct file **result)
{
    (*result) = kmalloc(sizeof(struct file));
	if (!(*result))
		return -ERR_NO_MEM;

	memcpy((char *)*result, (char *)this_file, sizeof(struct file));
	(*result)->owner = for_process;
	return RET_OK;
}

hret_t ext2_write(struct file *this_file, char *src_buf, size_t *len)
{
    struct ext2_memory_super *ext2_s = this_file->inode->super->custom_data;
    struct ext2_inode *ext2_i = this_file->inode->private_data;
    uint32_t byte_to_copy = 0;    
    size_t len_writed = 0;
    char *buffer = NULL;    
    hret_t ret = RET_OK;
    uint32_t block = 0;

    if((this_file->offset + *len) > ext2_i->size)
    {
        ext2_set_inode_size(ext2_i, this_file->offset + *len);
        mark_inode_as_dirty(this_file->inode);                      // Indique l'inode comme modifié
    }

    while(len_writed < *len)
    {
        block = this_file->offset / ext2_s->block_size;

		if((this_file->offset % ext2_s->block_size) != 0)	// Ecriture au milieu d'un bloc
		{
			ret = ext2_read_block(this_file->inode, block, &buffer);

			memcpy(buffer + (this_file->offset % ext2_s->block_size), src_buf, 
				   ((*len - len_writed) >= (ext2_s->block_size - (this_file->offset % ext2_s->block_size))) ? ext2_s->block_size - (this_file->offset % ext2_s->block_size) : *len - len_writed);	
		}
		else if((*len - len_writed) < ext2_s->block_size)		// Ecriture partiel d'un bloc
		{
			ret = ext2_read_block(this_file->inode, block, &buffer);
            	
			memcpy(buffer, src_buf, *len - len_writed);	
		}
		else
		{
            buffer = kmalloc(ext2_s->block_size);
            if(buffer == NULL)
            {
                ret = -ERR_NO_MEM;
                break;
            }

            memcpy(buffer, src_buf, ext2_s->block_size);		// Ecriture totale d'un block
		}

		ret = ext2_write_block(this_file->inode, block, buffer);
		if(ret != RET_OK)
			break;
			
		byte_to_copy = ((*len - len_writed) >= (ext2_s->block_size - (this_file->offset % ext2_s->block_size))) ? ext2_s->block_size - (this_file->offset % ext2_s->block_size) : *len - len_writed;
			
		this_file->offset += byte_to_copy;
		src_buf += byte_to_copy;
        len_writed += byte_to_copy;

        if(buffer)
            kfree(buffer);
        buffer = NULL;
    }

    *len = len_writed;                                                      // Indique la taille des données lues
    return ret;
}

hret_t ext2_read(struct file *this_file, char *dst_buf, size_t *len)
{
    struct ext2_memory_super *ext2_s = (struct ext2_memory_super *)this_file->inode->super->custom_data;
    struct ext2_inode *ext2_i = (struct ext2_inode *)this_file->inode->private_data;
    uint32_t byte_to_copy = 0;    
    size_t len_readed = 0;
    char *buffer = NULL;    
    hret_t ret = RET_OK;
    uint32_t block = 0;

    // On s'assure que l'on ne dépassera pas la fin du fichier
    if((uint32_t)this_file->offset + *len > ext2_i->size)
        *len = ext2_i->size - (uint32_t)this_file->offset;

    while(len_readed < *len)
    {
        block = this_file->offset / ext2_s->block_size;
        
        ret = ext2_read_block(this_file->inode, block, &buffer);            // Lit le block (+ alloue autoimatiquement le buffer)
        if(ret != RET_OK)
            break;
          
        /* Recopie la bonne partie du bloc et vérifie qu'il n'y aura pas d'overflow */
        byte_to_copy = ((*len - len_readed) >= (ext2_s->block_size - (this_file->offset % ext2_s->block_size))) ? ext2_s->block_size - (this_file->offset % ext2_s->block_size) : *len - len_readed;

		memcpy(dst_buf, buffer + (this_file->offset % ext2_s->block_size), byte_to_copy);
        
        // Actualise les variables pour la prochaine lecture
        this_file->offset += byte_to_copy;      
        len_readed += byte_to_copy;
        dst_buf += byte_to_copy;
        
        kfree(buffer);                                                      // Libère le buffer automatiquement alloué
    }

    *len = len_readed;                                                      // Indique la taille des données lues
    return ret;
    
}   

hret_t ext2_seek(struct file *this_file, seek_t whence, off_t offset, off_t *result)
{
    struct ext2_inode *ext2_i = this_file->inode->private_data;
	off_t starting_offset;

    // On peut se positionner dans un fichier uniquement
	if (this_file->inode->type != INODE_FILE)
		return -ERR_ACCES;

	switch (whence)
	{
		case SEEK_CUR:
			starting_offset = this_file->offset;
			break;

		case SEEK_END:
			starting_offset = ext2_i->size;
			break;

		case SEEK_SET:
			starting_offset = 0;
			break;

		default:
			return -ERR_BAD_ARG;
			break;
	}

	if (offset < -starting_offset)			        // Si l'offset se positionne avant le début du fichier (le résultat sera négatif)
		return -ERR_BAD_ARG;				        // On retourne une erreur

                                                    
    if((starting_offset + offset) > 4294967295)     // Si on dépasse la taille maximum d'un fichier ext2
        return -ERR_BAD_ARG;                        // On retourne une erreur

    // Pas besoin de faire autre chose car l'ext2 autorise les trous de fichier, il
    // suffit juste d'actualiser la taille du fichier
    this_file->offset = starting_offset + offset;
    *result = this_file->offset;
    
    if((uint32_t)this_file->offset > ext2_i->size)    // Si on dépasse la fin du fichier
    {
        ext2_set_inode_size(ext2_i, (uint32_t)this_file->offset);   // On met à jour l'inode en mémoire
        mark_inode_as_dirty(this_file->inode);                      // Indique l'inode comme modifié
    }
        
	return RET_OK;
}

hret_t ext2_close_file(struct inode _unused *this_inode, struct file *to_close)
{
    kfree(to_close);            // Libère la mémoire
    return RET_OK;
}

hret_t ext2_inode_destructor(struct inode *to_del)
{
    kfree(to_del->private_data);
    kfree(to_del);

    return RET_OK;
}

hret_t ext2_set_inode_size(struct ext2_inode *ext2_i, uint32_t newsize)
{
    ext2_i->nb_blocks = 0;
    ext2_i->size = newsize;

    if(newsize == 0)
        return RET_OK;

    if((newsize % 512) == 0)
        ext2_i->nb_blocks = (newsize / 512);
    else
        ext2_i->nb_blocks = (newsize / 512) + 1;

    return RET_OK;
}
