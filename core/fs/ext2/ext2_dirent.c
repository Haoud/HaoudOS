#include <lib/stdio.h>
#include <lib/string.h>
#include <lib/stdlib.h>
#include <core/mm/heap.h>
#include <core/fs/ext2/ext2.h>
#include <core/dev/block/block.h>

hret_t ext2_link(struct inode *this_inode, const struct process _unused *actor, const char *name, struct inode *inode_to_link)
{
    struct ext2_memory_super *ext2_s = this_inode->super->custom_data;
    struct ext2_inode *ext2_file = inode_to_link->private_data;    
    struct ext2_inode *ext2_i = this_inode->private_data;

    struct ext2_dirent *new_ext2_dir = NULL;    
    struct ext2_dirent *ext2_dir = NULL;        // Entrée de répertoire
    uint32_t dirent_real_size = 0;              // Taille réel d'une entrée de répertoire    
    uint32_t size_to_alloc = 0;                 // Taile total du futur ext2_dirent
    uint32_t name_lenght = strlen(name);        // Taille de la variable name sans le \0 de fin
    char *buffer = NULL;
    hret_t ret = RET_OK;

    uint32_t offset_in_block = 0;    
    uint32_t readed_size = 0;

    size_to_alloc = name_lenght + EXT2_DIRENT_BASE_SIZE;

    if((size_to_alloc % 4) != 0)
        size_to_alloc += 4 - (size_to_alloc % 4);     // On aligne la structure sur 4 octet
    
    if(!(ext2_i->mode & EXT2_INODE_DIR))
        return -ERR_NO_DIR;

    while(readed_size < ext2_i->size)
    {
        offset_in_block = 0;
        ret = ext2_read_block(this_inode, readed_size / ext2_s->block_size, &buffer);

        if(ret != RET_OK)
            return ret;

        ext2_dir = (struct ext2_dirent *)buffer;

        while(offset_in_block < ext2_s->block_size)
        {
            if(readed_size + offset_in_block > ext2_i->size)
                goto while_out_error;

            dirent_real_size = EXT2_DIRENT_BASE_SIZE + ext2_dir->name_lenght;

            if((dirent_real_size % 4) != 0)
                dirent_real_size += 4 - (dirent_real_size % 4);

            if((ext2_dir->entry_lenght - dirent_real_size) >= size_to_alloc)
            {
                offset_in_block += dirent_real_size;  
                goto while_out_ok;                
            }

            offset_in_block += ext2_dir->entry_lenght;  
            ext2_dir = (struct ext2_dirent *) (((char *)ext2_dir) + ext2_dir->entry_lenght);
        }

        readed_size += ext2_s->block_size;
        kfree(buffer);
    }
while_out_error:
    panic("Allouer un bloc");

while_out_ok:
    if(readed_size > ext2_s->block_size)
        panic("Répertoire ext2 avec plus d'un bloc de données");

    new_ext2_dir = (struct ext2_dirent *) (((char *)ext2_dir) + dirent_real_size);
    memset(new_ext2_dir, 0, size_to_alloc);

    new_ext2_dir->inode = inode_to_link->inode_id;
    new_ext2_dir->name_lenght = name_lenght;
    new_ext2_dir->entry_lenght = ext2_dir->entry_lenght - (uint16_t)dirent_real_size;
    new_ext2_dir->file_type = ext2_dirent_convert_vfs_type(inode_to_link->type);

    // Copie le nom
    memcpy(&new_ext2_dir->name, name, name_lenght);

    ext2_dir->entry_lenght = dirent_real_size;

    if(inode_to_link->type == INODE_DIRECTORY && ext2_file->size == 0)
        ext2_init_dir(this_inode->super, inode_to_link, this_inode);

    ext2_file->link_count++;  

    ret = ext2_write_block(this_inode, readed_size / ext2_s->block_size, buffer);
    if(ret != RET_OK)
        goto free_mem;

    ret = RET_OK;

free_mem:
    if(buffer)
        kfree(buffer);

    return ret;
}

hret_t ext2_lookup(struct inode *this_inode, const char *name, uint64_t *result_inode_id)
{
    struct ext2_memory_super *ext2_s = this_inode->super->custom_data;
    struct ext2_inode *ext2_i = this_inode->private_data;

    struct ext2_dirent *ext2_dir = NULL;   // Entrée de répertoire
    char *buffer = NULL;
    hret_t ret = RET_OK;

    uint32_t offset_in_block = 0;    
    uint32_t readed_size = 0;
    
    if(!(ext2_i->mode & EXT2_INODE_DIR))
        return -ERR_NO_DIR;

    while(readed_size < ext2_i->size)
    {
        offset_in_block = 0;
        ret = ext2_read_block(this_inode, readed_size / ext2_s->block_size, &buffer);

	if(ret != RET_OK)
		goto out;

        ext2_dir = (struct ext2_dirent *)buffer;

        while(offset_in_block < ext2_s->block_size)
        {
            if((offset_in_block + ext2_dir->name_lenght) > ext2_s->block_size)
                panic("Entrée de répertoire chevauché à travers différents blocks");

            if(readed_size > ext2_i->size)
                goto out;

            if(ext2_name_match(ext2_dir->name, name, ext2_dir->name_lenght))
            {
                *result_inode_id = ext2_dir->inode;  

                kfree(buffer);              
                return ret;
            }

            // On passe à la prochaine entrée
            offset_in_block += ext2_dir->entry_lenght;            
            ext2_dir = (struct ext2_dirent *) ((char *)ext2_dir + ext2_dir->entry_lenght);
        }

        readed_size += ext2_s->block_size;
        kfree(buffer);
    }
    
out:
    if(buffer)
        kfree(buffer);
    
    *result_inode_id = 0;
    return -ERR_NO_ENTRY;
}

hret_t ext2_unlink(struct inode *this_inode, struct process _unused *actor, const char *name)
{
    // On essaye de trouver l'entrée de répertoire
    struct ext2_memory_super *ext2_s = this_inode->super->custom_data;
    struct ext2_inode *ext2_i = this_inode->private_data;
    struct ext2_inode *ext2_to_remove =  NULL;    
    struct inode *inode_to_remove = NULL;

    struct ext2_dirent *prev_ext2_dir = NULL;   // Pour supprimer l'ntrée de répertoire
    struct ext2_dirent *ext2_dir = NULL;        // Entrée de répertoire
    char *buffer = NULL;
    hret_t ret;

    uint32_t offset_in_block = 0;    
    uint32_t readed_size = 0;
    
    if(!(ext2_i->mode & EXT2_INODE_DIR))
        return -ERR_NO_DIR;

    /* Impossible de supprimer l'entrée de répertoire . */
    if(strcmp(name, ".") == 0)
        return -ERR_BAD_ARG;
    
    /* Impossible de supprimer l'entrée de répertoire .. */
    if(strcmp(name, "..") == 0)
        return -ERR_BAD_ARG;

    while(readed_size < ext2_i->size)
    {
        offset_in_block = 0;
        ret = ext2_read_block(this_inode, readed_size / ext2_s->block_size, &buffer);

        ext2_dir = (struct ext2_dirent *)buffer;

        while(offset_in_block < ext2_s->block_size)
        {
            if((offset_in_block + ext2_dir->name_lenght) > ext2_s->block_size)
                panic("Entrée de répertoire chevauché à travers différents blocks");

            if(readed_size > ext2_i->size)
                break;

            if(ext2_name_match(ext2_dir->name, name, ext2_dir->name_lenght))
            {
                /*
                * On ne vérifie pas si prev_ext2_dir est nul car on ne peut pas supprimer les entrées . et ..,
                * qui sont toujours crée lors de la création du répertoire. Pour supprimer une entrée de
                * répertoire, il suffit d'augmenter la variable entry_lengh de l'entrée de répertoire précédente
                * du nombre contenu dans la variable entry_lengh de l'entrée à supprimer
                */
                prev_ext2_dir->entry_lenght += ext2_dir->entry_lenght;      // Simple, non ?
                ret = ext2_write_block(this_inode, readed_size / ext2_s->block_size, buffer);
                if(ret != RET_OK)
                    return ret;

                // Lit l'inode que l'on vient de supprimer du répertoire
                ret = fetch_inode(this_inode->super, ext2_dir->inode, &inode_to_remove);
                if(ret != RET_OK)
                    return ret;

                ext2_to_remove = (struct ext2_inode *) inode_to_remove->private_data;

                // Décrémente son champ link count et réécrit l'inode sur le disque
                inode_to_remove->disk_count--;
                ext2_to_remove->link_count--;

                mark_inode_as_dirty(inode_to_remove);
                // Si le champ link count devient nul alors on supprime l'inode et son contenu
                // TODO: le faire

                kfree(buffer);              
                return ret;
            }
            // On passe à la prochaine entrée
            offset_in_block += ext2_dir->entry_lenght;    
            
            prev_ext2_dir = ext2_dir;
            ext2_dir = (struct ext2_dirent *) ((char *)ext2_dir + ext2_dir->entry_lenght);
        }

        prev_ext2_dir = NULL;
        readed_size += ext2_s->block_size;
        kfree(buffer);
    }

    if(buffer)
        kfree(buffer);

    return -ERR_NO_ENTRY;
}

hret_t ext2_readdir(struct file *this_file, struct dirent *result)
{
    struct ext2_inode *ext2_file = NULL;
    struct inode *inode_file = NULL;
    hret_t ret;
        
    do
    {
        // Lecture de l'entrée du répertoire
        ret = ext2_read_vfs_direntry(this_file->inode, this_file->offset, result);
        if(ret != RET_OK)
            goto free_mem;
            
        // Lecture de l'inode associé
        if(ext2_file)
            kfree(ext2_file);

        ret = fetch_inode(this_file->inode->super, result->inode_id, &inode_file);

        if(ret != RET_OK)
            goto free_mem;
            
        ext2_file = inode_file->private_data;
        
        this_file->offset++;   
    }while(ext2_file->creation_time > this_file->generation);
    
    // La structure result est à jour sauf offset_in_dirfile
    result->offset_in_dirfile = this_file->offset;
    goto out;

free_mem:
    if(ext2_file)
        kfree(ext2_file);

    if(ret == -ERR_NO_ENTRY)        // Si nous sommes arrivé à la fin du répertoire
        this_file->offset = 0;      // On se replace à zéro
    
out:
	return ret;
}

/*
 * dirent_id = numéro d'entrée de répertoire (1er, 2ème...) en partant du début
 */
hret_t ext2_read_vfs_direntry(struct inode *inode, uint32_t dirent_id, struct dirent *result)
{
    struct ext2_memory_super *ext2_s = (struct ext2_memory_super *) inode->super->custom_data;
    struct ext2_inode *ext2_i = inode->private_data;
    struct ext2_dirent *ext2_d = NULL;
    uint32_t offset_in_block = 0;    
    uint32_t readed_size = 0;
    char *buffer = NULL;
    uint32_t i = 0;    
    hret_t ret;

   /* ret = fetch_inode(inode->super, inode->inode_id, &vfs_inode);
    if(ret != RET_OK)
        goto free_mem;

    ext2_i = (struct ext2_inode *) vfs_inode->private_data;*/

    while(readed_size < ext2_i->size)
    {
        offset_in_block = 0;
        ret = ext2_read_block(inode, readed_size / ext2_s->block_size, &buffer);
                
        ext2_d = (struct ext2_dirent *)buffer;

        while(offset_in_block < ext2_s->block_size)
        {
            if((offset_in_block + ext2_d->name_lenght) > ext2_s->block_size)
                panic("Entrée de répertoire chevauché à travers différents blocks");

            // Si on dépasse la taille du répertoire
            if(readed_size > ext2_i->size)
                goto no_entry;                          // Quitte la boucle

            if(i == dirent_id)
            {
                memcpy(result->name, &ext2_d->name, ext2_d->name_lenght);
                result->name[ext2_d->name_lenght] = 0;

                result->inode_id = ext2_d->inode;
                result->type = ext2_convert_dirent_type(ext2_d->file_type);
                result->offset_in_dirfile = 0;

                kfree(buffer);
                return RET_OK;
            }

            // On passe à la prochaine entrée
            i++;            
            offset_in_block += ext2_d->entry_lenght;            
            ext2_d = (struct ext2_dirent *) ((char *)ext2_d + ext2_d->entry_lenght);
        }

        kfree(buffer);        
        readed_size += ext2_s->block_size;
    }

no_entry:
    ret = -ERR_NO_ENTRY;
    goto free_mem;
free_mem:
    if(buffer)
        kfree(buffer);
    return ret;
}

hret_t ext2_init_dir(struct super_block *super, struct inode *dir, struct inode *parent)
{
    // On crée dans un buffer les entrées . et ..
    struct ext2_memory_super *ext2_s = (struct ext2_memory_super *)super->custom_data;
    struct ext2_inode *ext2_i = (struct ext2_inode *)dir->private_data;
    struct ext2_group_descriptor *gdesc = NULL;
    struct ext2_dirent *dot, *dotdot;
    uint32_t group_num = 0;
    char *buffer;
    hret_t ret;

    if(ext2_i->size != 0)
        return -ERR_NOT_EMPTY;

    buffer = kmalloc(ext2_s->block_size);
    if(buffer == NULL)
        return -ERR_NO_MEM;        

    ret = ext2_set_inode_size(ext2_i, ext2_s->block_size);
    if(ret != RET_OK)
        goto out;

    dot = (struct ext2_dirent *)buffer;        
    dot->file_type = ext2_dirent_convert_vfs_type(dir->type);        
    dot->inode = dir->inode_id;
    dot->entry_lenght = 12;             // Aligné sur 4 octet
    dot->name_lenght = 1;
    dot->name[0] = '.';
    dot->name[1] = '\0';
    dot->name[2] = '\0';
    dot->name[3] = '\0';

    
    dotdot = (struct ext2_dirent *)(buffer + 12);
    dotdot->file_type = ext2_dirent_convert_vfs_type(parent->type);    
    dotdot->inode = parent->inode_id;
    dotdot->entry_lenght = ext2_s->block_size - 12;
    dotdot->name_lenght = 2;
    dotdot->name[0] = '.';
    dotdot->name[1] = '.';
    dotdot->name[2] = '\0';
    dotdot->name[3] = '\0';

    group_num = ((uint32_t)dir->inode_id - 1) / ext2_s->super.inodes_per_group;             // On localise dans quel groupe se situe l'inode

    ret = ext2_read_group_descriptor(dir->super, group_num, &gdesc);
    if(ret != RET_OK)
        goto out;

    gdesc->used_dirs_count++;
    ret = ext2_write_group_descriptor(dir->super, group_num, gdesc);
    if(ret != RET_OK)
        goto out;

    ret = ext2_write_block(dir, 0, buffer);
out:
    kfree(buffer);
    return ret;
}

enum inode_type ext2_convert_dirent_type(uint8_t ext2_dirent_type)
{
    if(ext2_dirent_type == EXT2_DIRENT_REG)
        return INODE_FILE;
    else if(ext2_dirent_type == EXT2_DIRENT_DIR)
        return  INODE_DIRECTORY;
    else if(ext2_dirent_type == EXT2_DIRENT_CHAR)
        return INODE_CHAR_DEVICE;
    else if(ext2_dirent_type == EXT2_DIRENT_BLOCK)
        return INODE_BLOCK_DEVICE;
    else if(ext2_dirent_type == EXT2_DIRENT_SYMBOLIC_LINK)
        return INODE_SYMBOLIC_LINK;
    else
        return INODE_FILE;    
}

uint8_t ext2_dirent_convert_vfs_type(enum inode_type vfs_type)
{
    if(vfs_type == INODE_FILE)
        return EXT2_DIRENT_REG;
    else if(vfs_type == INODE_DIRECTORY)
        return EXT2_DIRENT_DIR;
    else if(vfs_type == INODE_CHAR_DEVICE)
        return EXT2_DIRENT_CHAR;
    else if(vfs_type == INODE_BLOCK_DEVICE)
        return EXT2_DIRENT_BLOCK;
    else if(vfs_type == INODE_SYMBOLIC_LINK)
        return EXT2_DIRENT_SYMBOLIC_LINK;
    else
        return EXT2_DIRENT_REG;    
}

bool_t ext2_name_match(const char *ext2_name, const char *user_name, int max_count)
{
    if (max_count == 0)
		return TRUE;

	while (max_count-- != 0)
    {
		if (*ext2_name++ != *user_name++)
			return FALSE;
    }
	
	if(*user_name != '\0')
		return FALSE;

	return TRUE;
}
