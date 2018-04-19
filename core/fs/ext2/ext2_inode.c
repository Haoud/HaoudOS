#include <const.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/mm/heap.h>
#include <core/time/time.h>
#include <core/fs/ext2/ext2.h>
#include <core/dev/block/block.h>

hret_t ext2_allocate_inode(struct super_block *super, enum inode_type type, struct inode **result)
{
    struct ext2_inode *ext2_i = NULL;
    uint32_t result_inode_id = 0;
    hret_t ret;
    
    ret = ext2_get_free_inode(super, &result_inode_id);
    if(ret != RET_OK)
        return ret;

    ret = ext2_get_vfs_inode(super, result_inode_id, result);
    if(ret != RET_OK)
        goto free_mem;

    ext2_i = (struct ext2_inode *) (*result)->private_data;

    // Ici, l'inode est vide, il faut le remplir
    ext2_i->mode = ext2_inode_convert_vfs_type(type); 

    ext2_i->creation_time = get_current_unix_time(); 
    ext2_i->last_acces_time = get_current_unix_time();              
    ext2_i->last_modif_time = get_current_unix_time();              
    ext2_i->delete_time = 0;     
    
    ext2_i->size = 0;    
    ext2_i->nb_blocks = 0;
    memset(ext2_i->block, 0, sizeof(uint32_t) * EXT2_INODE_MAX_BLOCKS);

    // Champs non supportés par HaoudOS
    ext2_i->gid = 0;
    ext2_i->uid = 0;
    ext2_i->osd1 = 0;    
    ext2_i->flags = 0;    
    ext2_i->dir_acl = 0;
    ext2_i->file_acl = 0;
    ext2_i->version = 0;    
    ext2_i->fragment_addr = 0;
    ext2_i->link_count = 0; 
    
    return RET_OK;

free_mem:
    if(ext2_i)
        kfree(ext2_i);

    result = NULL;
    return ret;
}

hret_t ext2_fetch_inode(struct super_block *this_super, uint64_t inode_id, struct inode **result)
{
    return ext2_get_vfs_inode(this_super, (uint32_t)inode_id, result);
}

hret_t ext2_stat(struct inode * this_inode, struct stat *result)
{
    struct ext2_memory_super *ext2_s = (struct ext2_memory_super *)this_inode->super->custom_data;
    struct ext2_inode *ext2_i = (struct ext2_inode *)this_inode->private_data;

    result->blksize = ext2_s->block_size;
    result->dev = this_inode->super->dev;
    result->ino = this_inode->inode_id;
    result->nlink = ext2_i->link_count;
    result->type = this_inode->type;
    result->size = ext2_i->size;

    if(this_inode->type == INODE_CHAR_DEVICE || this_inode->type == INODE_BLOCK_DEVICE)
        result->rdev = this_inode->dev;
    else
        result->rdev = 0;
    
    return RET_OK;
}

hret_t ext2_truncate(struct inode *this_inode, off_t length)
{
    struct ext2_memory_super *ext2_s = this_inode->super->custom_data;    
    struct ext2_inode *ext2_i = this_inode->private_data;
    uint32_t block_to_release = 0;
    uint32_t block_id = 0;
    hret_t ret = RET_OK;

    if(length == ext2_i->size)
        return RET_OK;

    if(length > ext2_i->size)
    {
        ext2_i->size = length;
        return RET_OK;
    }

    // Ici, il faut réduire la taille du fichier
    //On libère les blocks
    if((length % ext2_s->block_size) == 0)
        block_to_release = length / ext2_s->block_size;
    else
        block_to_release = (length / ext2_s->block_size) + 1;

    for(; block_to_release <= (ext2_i->size / ext2_s->block_size); block_to_release++)
    {
        ret = ext2_get_inode_block(this_inode, block_to_release, &block_id);
        if(ret != RET_OK)
            continue;           // On ignore l'erreur

        if(block_id == 0)       // Pas de block alloué
            continue;           // Donc pas besoin de le libérer

        ret = ext2_release_block(this_inode->super, block_id);
    }

    ext2_set_inode_size(ext2_i, length);
    mark_inode_as_dirty(this_inode);

    return RET_OK;
}

hret_t ext2_sync(struct inode *this_inode)
{
    struct ext2_memory_super *ext2_s = this_inode->super->custom_data;    
    struct ext2_inode *ext2_i = this_inode->private_data;
    hret_t ret = RET_OK;

	// Simplification parfois incorrecte ?
	if(this_inode->type == INODE_BLOCK_DEVICE || this_inode->type == INODE_CHAR_DEVICE)
		ext2_i->block[0] = this_inode->dev;
    
    if(ext2_s->is_dirty)
        ret = ext2_write_super(ext2_s, this_inode->super->dev);

    if(ret != RET_OK)
        ext2_write_inode(this_inode->super, this_inode->inode_id, ext2_i);
    else
        ret = ext2_write_inode(this_inode->super, this_inode->inode_id, ext2_i);

    return ret;
}

/*
 * Les fonction ci dessous sont des fonctions utilitaire du driver ext2 et ne sont pas requise pour
 * les VFS: leurs prototypes peuvent être modifiés sans problèmes ni grandes difficultés
 */
hret_t ext2_get_vfs_inode(struct super_block *super, uint32_t inode_nr, struct inode **result)
{
    struct ext2_inode *ext2_i = NULL;
    hret_t ret = RET_OK;

    (*result) = NULL;

     // Lit l'inode désiré
    ret = ext2_read_inode(super, inode_nr, &ext2_i);            // C'est le seul endroit à l'on peut utiliser ext2_read_inode
    if(ret != RET_OK)
        return ret;

    // Alloue le buffer pour l'inode du VFS
    (*result) = kmalloc(sizeof(struct inode));                  
    if((*result) == NULL)
        goto no_mem;
    
    // Remplit une structure inode
    (*result)->disk_count = ext2_i->link_count;					// Nombre de référence à l'inode sur le disque (l'inode est créé mais pas référencé)
	(*result)->memory_count = 1;								// L'inode est pour l'instant référencé 1 fois
	(*result)->generation = 0;									// Pour éviter des problènes de désynchronisation
	(*result)->inode_id = (uint64_t)inode_nr;		            // Pour pouvoir récupérer l'inode à partir de son ID
	(*result)->is_dirty = FALSE;								// L'inode est modifié (mais ne change pas grand chose ici)
	(*result)->private_data = ext2_i;						    // Pointeur vers l'inode ext2
	(*result)->super = super;								    // Super block actuel 
	(*result)->inode_op = &ext2_inode_op;					    // Opérations pour tous les inodes

    // Convertit le type d'inode ext2 en type d'inode du VFS
    if(ext2_i->mode & EXT2_INODE_REG)
        (*result)->type = INODE_FILE;
    else if(ext2_i->mode & EXT2_INODE_DIR)
        (*result)->type = INODE_DIRECTORY;
    else if(ext2_i->mode & EXT2_INODE_CHAR)
        (*result)->type = INODE_CHAR_DEVICE;
    else if(ext2_i->mode & EXT2_INODE_BLOCK)
        (*result)->type = INODE_BLOCK_DEVICE;
    else if(ext2_i->mode & EXT2_INODE_SYMBOLIC_LINK)
        (*result)->type = INODE_SYMBOLIC_LINK;
    else
        (*result)->type = INODE_FILE;    

    (*result)->inode_op = &ext2_inode_op;                       // Commun à tout les inodes

    if((*result)->type == INODE_DIRECTORY)                      
        (*result)->dir_op = &ext2_dir_op;
    if((*result)->type == INODE_CHAR_DEVICE || (*result)->type == INODE_BLOCK_DEVICE)
    {
        // ATTENTION: JE NE SUIS PAS SUR DE CET ALGORITHME CAR JE N'AI PAS TROUVE DE
        // DOCUMENTATIONS SUR CE SUJET. JE ME SUIS BASE SUR MES PROPRES TESTS :-/

        if(ext2_i->block[0] != 0)
            (*result)->dev = MAKE_DEV((ext2_i->block[0] & 0x00000F00) >> 8, ext2_i->block[0] & 0x0000000F);            
        else
            (*result)->dev = MAKE_DEV((ext2_i->block[1] & 0x00FFF000) >> 12, ext2_i->block[1] & 0x00000FFF);   
            
    #ifdef DEBUG_MODE
        printk("Warning: ext2 device are buggy\n", (*result)->dev);
    #endif
    }

    // Operation sur l'inode du vfs en mémoire
    (*result)->open_file = ext2_open_file;              // Lorsque l'on ouvre le fichier associé à l'inode
    (*result)->close_file = ext2_close_file;            // Lorsque l'on ferme le fichier associé à l'inode
    (*result)->destructor = ext2_inode_destructor;      // Lorsque l'inode est détruit de la mémoire
    
    goto out;

no_mem:
    if(ext2_i)
        kfree(ext2_i);

    if(*result)
        kfree(result);
    return -ERR_NO_MEM;
out:
    return RET_OK;
}

hret_t ext2_write_inode(struct super_block *super, uint32_t inode_nr, struct ext2_inode *to_write)
{
    struct ext2_memory_super *ext2_super = (struct ext2_memory_super *) super->custom_data;
    size_t len_to_write = sizeof(struct ext2_inode);    
    struct ext2_group_descriptor *gdesc = NULL;
    uint32_t group_index = 0;   
    off_t inode_location = 0;    
    uint32_t group_num = 0;
    hret_t ret = RET_OK;

    if(inode_nr > ext2_super->super.inodes_count)
        return -ERR_BAD_ARG;

    // Calcul de la location de l'inode
    group_num = (inode_nr - 1) / ext2_super->super.inodes_per_group;             // On localise dans quel groupe se situe l'inode
    group_index = (inode_nr - 1) % ext2_super->super.inodes_per_group;           // Puis on localise l'inode dans ce groupe

    //On lit le groupe descriptor
    ret = ext2_read_group_descriptor(super, group_num, &gdesc);
    if(ret != RET_OK)
        goto out;

    inode_location = gdesc->inode_table * ext2_super->block_size + group_index * ext2_super->super.inode_size;

    // On écrit l'inode sur le disque
    ret = blockdev_kernel_write(lookup_blockdev(super->dev), inode_location, to_write, &len_to_write);

    if(ret != RET_OK)
        goto out;
    
    if(len_to_write != sizeof(struct ext2_inode))
        goto io_error;

    goto out;

io_error:
    ret = -ERR_IO;
out:
    return ret;
}

hret_t ext2_read_inode(struct super_block *super, uint32_t inode_nr, struct ext2_inode **result)
{
    struct ext2_memory_super *ext2_super = (struct ext2_memory_super *) super->custom_data;
    size_t len_to_read = sizeof(struct ext2_inode);    
    struct ext2_group_descriptor *gdesc = NULL;
    uint32_t group_index = 0;   
    off_t inode_location = 0;    
    uint32_t group_num = 0;
    hret_t ret = RET_OK;

    if(inode_nr > ext2_super->super.inodes_count)
        return -ERR_BAD_ARG;

    (*result) = kmalloc(sizeof(struct ext2_inode));
    if((*result) == NULL)
        return -ERR_NO_MEM;

    // Calcul de la location de l'inode
    group_num = (inode_nr - 1) / ext2_super->super.inodes_per_group;             // On localise dans quel groupe se situe l'inode
    group_index = (inode_nr - 1) % ext2_super->super.inodes_per_group;           // Puis on localise l'inode dans ce groupe

    //On lit le groupe descriptor
    ret = ext2_read_group_descriptor(super, group_num, &gdesc);
    if(ret != RET_OK)
        goto free_mem;

    inode_location = gdesc->inode_table * ext2_super->block_size + group_index * ext2_super->super.inode_size;

    // On lit l'inode sur le disque
    ret = blockdev_kernel_read(lookup_blockdev(super->dev), inode_location, *result, &len_to_read);

    if(ret != RET_OK)
        goto free_mem;
    
    if(len_to_read != sizeof(struct ext2_inode))
        goto io_error;

    goto out;

io_error:
    ret = -ERR_IO;
free_mem:
    kfree(*result);
out:
    return ret;
}

/*
 * Cette fonction permet de lire un bloc identifié par son index dans le fichier
 * et s'occupe de tout: même de l'allocation du buffer
 */  
hret_t ext2_read_block(struct inode *i, uint32_t block_index, char **buffer)
{
    struct ext2_memory_super *ext2_s = i->super->custom_data;    
    struct ext2_inode *ext2_i = i->private_data;
    size_t len_to_read = ext2_s->block_size;        
    uint32_t *indirect1 = NULL;                     // Bloc d'indirection simple
    uint32_t *indirect2 = NULL;                     // Bloc d'indirection double
    uint32_t *indirect3 = NULL;                     // Bloc d'indirection triple
    uint64_t disk_offset;
    hret_t ret = RET_OK;

    if((block_index * ext2_s->block_size) > ext2_i->size)
        return -ERR_BAD_ARG;

    (*buffer) = kmalloc(ext2_s->block_size);
    if((*buffer) == NULL)
        goto no_mem;
        
    memset(*buffer, 0, ext2_s->block_size);

    if(EXT2_NEED_INDIRECT1(ext2_s, block_index))
    {
        /* On doit lire un bloc d'indirection simple */
        // On localise le block indirect simple
        indirect1 = kmalloc(ext2_s->block_size);
        if(indirect1 == NULL)
            goto no_mem;

        disk_offset = ext2_i->block[12] * ext2_s->block_size;       // Emplacement du bloc d'indirection simple
        if(disk_offset == 0)
            goto break_if;
        
        // On lit le bloc indirect simple
        ret = blockdev_kernel_read(lookup_blockdev(i->super->dev), disk_offset, indirect1, &len_to_read);
        
        if(ret != RET_OK)
            goto free_mem;
    
        if(len_to_read != ext2_s->block_size)
            goto io_error;
            
        disk_offset = indirect1[block_index - 12] * ext2_s->block_size;
        if(disk_offset == 0)
            goto break_if;    

        // On lit le bloc concerné
        ret = blockdev_kernel_read(lookup_blockdev(i->super->dev), disk_offset, *buffer, &len_to_read);
            
        if(ret != RET_OK)
            goto free_mem;
        
        if(len_to_read != ext2_s->block_size)
             goto io_error;
    }
    else if(EXT2_NEED_INDIRECT2(ext2_s, block_index))
    {
        // Indirection double
        indirect1 = kmalloc(ext2_s->block_size);
        indirect2 = kmalloc(ext2_s->block_size);
        if(indirect1 == NULL || indirect2 == NULL)
            goto no_mem;

        disk_offset = ext2_i->block[13] * ext2_s->block_size;       // Emplacement du bloc d'indirection double
        if(disk_offset == 0)
            goto break_if;
        
        // On lit le bloc indirect simple
        ret = blockdev_kernel_read(lookup_blockdev(i->super->dev), disk_offset, indirect1, &len_to_read);
        
        if(ret != RET_OK)
            goto free_mem;
    
        if(len_to_read != ext2_s->block_size)
            goto io_error;

        block_index -= 12 + (ext2_s->block_size / 4);		
        disk_offset = indirect1[block_index / (ext2_s->block_size / 4)] * ext2_s->block_size;       // Emplacement du bloc d'indirection simple

        if(disk_offset == 0)
            goto break_if;
        
        // On lit le bloc indirect double
        ret = blockdev_kernel_read(lookup_blockdev(i->super->dev), disk_offset, indirect2, &len_to_read);
        
        if(ret != RET_OK)
            goto free_mem;
    
        if(len_to_read != ext2_s->block_size)
            goto io_error;
            
        disk_offset = indirect2[block_index % (ext2_s->block_size / 4)] * ext2_s->block_size;
        if(disk_offset == 0)
            goto break_if;    

        // On lit le bloc concerné
        ret = blockdev_kernel_read(lookup_blockdev(i->super->dev), disk_offset, *buffer, &len_to_read);
            
        if(ret != RET_OK)
            goto free_mem;
        
        if(len_to_read != ext2_s->block_size)
             goto io_error;
    }
    else if(EXT2_NEED_INDIRECT3(ext2_s, block_index))
    {
        // Indirection triple
        panic("Indrection triple non supportée");
    }
    else
    {
        // Pas d'indirection       
        disk_offset = ext2_i->block[block_index] * ext2_s->block_size;

        // Si on a affaire à un trou de fichier
        if(disk_offset == 0)
            return RET_OK;      // Pas de problème, le buffer est déjà mit à zéro

        ret = blockdev_kernel_read(lookup_blockdev(i->super->dev), disk_offset, *buffer, &len_to_read);

        if(ret != RET_OK)
            goto free_mem;

        if(len_to_read != ext2_s->block_size)
            goto io_error;
    }
    
break_if:
    ret = RET_OK;
    goto out;

no_mem:
    ret = -ERR_NO_MEM;
    goto free_mem;
io_error:
    ret = -ERR_IO;
free_mem:
    kfree(*buffer);
out:
    if(indirect1)
        kfree(indirect1);

    if(indirect2)
        kfree(indirect2);

    if(indirect3)
        kfree(indirect3);  
    return ret;
}

/*
 * Cette fonction permet d'écrire un bloc identifié par son index dans le fichier
 * et s'occupe de tout, et ajoute un nouveau bloc à l'inode s'il le faut
 * Cette fonction ne peut pas écrire en dehors de la taille du fichier, pour cela, il suffit
 * de modifier la taille du fichier contenant dans l'inode avant d'appeler cette fonction
 * Note: La taille du fichier doit permettre l'allocation du bloc
 */  
hret_t ext2_write_block(struct inode *i, uint32_t block_index, char *buffer)
{
    struct ext2_memory_super *ext2_s = i->super->custom_data;    
    struct ext2_inode *ext2_i = i->private_data;
    size_t len_to_write = ext2_s->block_size;      
    uint32_t alloc_block_index = 0;                 // S'il faut allouer un ou plusueirs bloc        
    uint32_t *indirect1 = NULL;                     // Bloc d'indirection simple
    uint32_t *indirect2 = NULL;                     // Bloc d'indirection double
    uint32_t *indirect3 = NULL;                     // Bloc d'indirection triple
    uint64_t disk_offset;
    hret_t ret = RET_OK;

    if((block_index * ext2_s->block_size) > ext2_i->size)
        return -ERR_BAD_ARG;

    if(EXT2_NEED_INDIRECT1(ext2_s, block_index))
    {
        /* On doit écrire à travers un bloc d'indirection simple */
        // On localise le block indirecte simple
        indirect1 = kmalloc(ext2_s->block_size);
        if(indirect1 == NULL)
            goto no_mem;

        disk_offset = ext2_i->block[12] * ext2_s->block_size;       // Emplacement du bloc d'indirection simple

        // Si le bloc n'est pas alloué
        if(disk_offset == 0)
        {
            // On alloue le bloc
            ret = ext2_get_free_block(i->super, &alloc_block_index);
            if(ret != RET_OK)
                goto free_mem;

            char *buffer = kmalloc(ext2_s->block_size);

            if(!buffer)
                goto free_mem;

            memset(buffer, 0, ext2_s->block_size);
            ret = blockdev_kernel_write(lookup_blockdev(i->super->dev), alloc_block_index * ext2_s->block_size, buffer, &len_to_write);
        
            if(ret != RET_OK)
                goto free_mem;

            if(len_to_write != ext2_s->block_size)
                goto io_error;

            kfree(buffer);

            // On met à jour l'inode puis on le sauvegarde sur le disque
            ext2_i->block[12] = alloc_block_index;

            // On calcul l'offset maintenant que le bloc est alloué
            disk_offset = ext2_i->block[12] * ext2_s->block_size;
        } 
        
        // On lit le bloc indirect simple
        ret = blockdev_kernel_read(lookup_blockdev(i->super->dev), disk_offset, indirect1, &len_to_write);
        
        if(ret != RET_OK)
            goto free_mem;
    
        if(len_to_write != ext2_s->block_size)
            goto io_error;
            
        disk_offset = indirect1[block_index - 12] * ext2_s->block_size;

        // Si le bloc n'est pas alloué
        if(disk_offset == 0)
        {
            // On alloue le bloc
            ret = ext2_get_free_block(i->super, &alloc_block_index);
            if(ret != RET_OK)
                goto free_mem;

			char *buffer = kmalloc(ext2_s->block_size);

            if(!buffer)
                goto free_mem;

            memset(buffer, 0, ext2_s->block_size);
            ret = blockdev_kernel_write(lookup_blockdev(i->super->dev), alloc_block_index * ext2_s->block_size, buffer, &len_to_write);

            // On met à jour l'inode puis on le sauvegarde sur le disque
            indirect1[block_index - 12] = alloc_block_index;
            ret = blockdev_kernel_write(lookup_blockdev(i->super->dev), ext2_i->block[12] * ext2_s->block_size, indirect1, &len_to_write);

            if(ret != RET_OK)
                goto free_mem;
        
            if(len_to_write != ext2_s->block_size)
                goto io_error;

            // On calcul l'offset maintenant que le bloc est alloué
            disk_offset = indirect1[block_index - 12] * ext2_s->block_size;
        } 
        
        // On écrit le block concerné
        ret = blockdev_kernel_write(lookup_blockdev(i->super->dev), disk_offset, buffer, &len_to_write);

        if(ret != RET_OK)
            goto free_mem;
        
        if(len_to_write != ext2_s->block_size)
             goto io_error;
    }
    else if(EXT2_NEED_INDIRECT2(ext2_s, block_index))
    {
		// Indirection double
        indirect1 = kmalloc(ext2_s->block_size);
        indirect2 = kmalloc(ext2_s->block_size);
        if(indirect1 == NULL || indirect2 == NULL)
            goto no_mem;

        disk_offset = ext2_i->block[13] * ext2_s->block_size;       // Emplacement du bloc d'indirection double
        if(disk_offset == 0)
        {
			// On alloue le bloc
            ret = ext2_get_free_block(i->super, &alloc_block_index);
            if(ret != RET_OK)
                goto free_mem;

            char *buffer = kmalloc(ext2_s->block_size);

            if(!buffer)
                goto free_mem;

            memset(buffer, 0, ext2_s->block_size);
            ret = blockdev_kernel_write(lookup_blockdev(i->super->dev), alloc_block_index * ext2_s->block_size, buffer, &len_to_write);
        
		    kfree(buffer);

            if(ret != RET_OK)
                goto free_mem;

            if(len_to_write != ext2_s->block_size)
                goto io_error;


            // On met à jour l'inode puis on le sauvegarde sur le disque
            ext2_i->block[13] = alloc_block_index;

            // On calcul l'offset maintenant que le bloc est alloué
            disk_offset = ext2_i->block[13] * ext2_s->block_size;
		}
        
        // On lit le bloc indirect double
        ret = blockdev_kernel_read(lookup_blockdev(i->super->dev), disk_offset, indirect1, &len_to_write);
        
        if(ret != RET_OK)
            goto free_mem;
    
        if(len_to_write != ext2_s->block_size)
            goto io_error;

        block_index -= 12 + (ext2_s->block_size / 4);		
        disk_offset = indirect1[block_index / (ext2_s->block_size / 4)] * ext2_s->block_size;       // Emplacement du bloc d'indirection double

        if(disk_offset == 0)
        {
			 // On alloue le bloc
            ret = ext2_get_free_block(i->super, &alloc_block_index);
            if(ret != RET_OK)
                goto free_mem;

			char *buffer = kmalloc(ext2_s->block_size);

            if(!buffer)
                goto free_mem;

            memset(buffer, 0, ext2_s->block_size);
            ret = blockdev_kernel_write(lookup_blockdev(i->super->dev), alloc_block_index * ext2_s->block_size, buffer, &len_to_write);

			kfree(buffer);

            // On met à jour l'inode puis on le sauvegarde sur le disque
            indirect1[block_index / (ext2_s->block_size / 4)] = alloc_block_index;
            ret = blockdev_kernel_write(lookup_blockdev(i->super->dev), ext2_i->block[13] * ext2_s->block_size, indirect1, &len_to_write);

            if(ret != RET_OK)
                goto free_mem;
        
            if(len_to_write != ext2_s->block_size)
                goto io_error;

            // On calcul l'offset maintenant que le bloc est alloué
            disk_offset = alloc_block_index * ext2_s->block_size;
		}
        
        // On lit le bloc indirect simple
        ret = blockdev_kernel_read(lookup_blockdev(i->super->dev), disk_offset, indirect2, &len_to_write);
        
        if(ret != RET_OK)
            goto free_mem;
    
        if(len_to_write != ext2_s->block_size)
            goto io_error;
            
        disk_offset = indirect2[block_index % (ext2_s->block_size / 4)] * ext2_s->block_size;

        if(disk_offset == 0)
        {
			// On alloue le bloc
            ret = ext2_get_free_block(i->super, &alloc_block_index);
            if(ret != RET_OK)
                goto free_mem;

			char *buffer = kmalloc(ext2_s->block_size);

            if(!buffer)
                goto free_mem;

            memset(buffer, 0, ext2_s->block_size);
            ret = blockdev_kernel_write(lookup_blockdev(i->super->dev), alloc_block_index * ext2_s->block_size, buffer, &len_to_write);

            kfree(buffer);

            // On met à jour l'inode puis on le sauvegarde sur le disque
            indirect2[block_index % (ext2_s->block_size / 4)] = alloc_block_index;
            ret = blockdev_kernel_write(lookup_blockdev(i->super->dev), indirect1[block_index / (ext2_s->block_size / 4)] * ext2_s->block_size, indirect2, &len_to_write);

            if(ret != RET_OK)
                goto free_mem;
        
            if(len_to_write != ext2_s->block_size)
                goto io_error;

            // On calcul l'offset maintenant que le bloc est alloué
            disk_offset = alloc_block_index * ext2_s->block_size;
		}    

        // On lit le bloc concerné
        ret = blockdev_kernel_write(lookup_blockdev(i->super->dev), disk_offset, buffer, &len_to_write);
            
        if(ret != RET_OK)
            goto free_mem;
        
        if(len_to_write != ext2_s->block_size)
             goto io_error;
    }
    else if(EXT2_NEED_INDIRECT3(ext2_s, block_index))
    {
        // Indirection triple
        panic("Indrection triple non supportée en écriture");
    }
    else
    {
        // Pas d'indirection    
        disk_offset = ext2_i->block[block_index] * ext2_s->block_size;
        
        // Si le bloc n'est pas alloué
        if(disk_offset == 0)
        {
            // On alloue le bloc
            ret = ext2_get_free_block(i->super, &alloc_block_index);
            if(ret != RET_OK)
                goto free_mem;

            // On met à jour l'inode puis on le sauvegarde sur le disque
            ext2_i->block[block_index] = alloc_block_index;

            // On calcul l'offset maintenant que le bloc est alloué
            disk_offset = ext2_i->block[block_index] * ext2_s->block_size;                
        } 

        ret = blockdev_kernel_write(lookup_blockdev(i->super->dev), disk_offset, buffer, &len_to_write);

        if(ret != RET_OK)
            goto free_mem;

        if(len_to_write != ext2_s->block_size)
            goto io_error;
    }
    
    ret = RET_OK;
	goto free_mem;

no_mem:
    ret = -ERR_NO_MEM;
    goto free_mem;
io_error:
    ext2_debugk("I/O error (ext2_write_block): incomplete writing (%u/%u o) (at block %u)\n", len_to_write, ext2_s->block_size, (uint32_t)(disk_offset / ext2_s->block_size));
    ret = -ERR_IO;
free_mem:
    if(indirect1)
        kfree(indirect1);

    if(indirect2)
        kfree(indirect2);

    if(indirect3)
        kfree(indirect3);  

    // if(Si un bloc a été alloué)
    // kfree(bloc alloué);

    return ret;
}

uint16_t ext2_inode_convert_vfs_type(enum inode_type vfs_type)
{
    if(vfs_type == INODE_FILE)
        return EXT2_INODE_REG;
    else if(vfs_type == INODE_DIRECTORY)
        return EXT2_INODE_DIR;
    else if(vfs_type == INODE_CHAR_DEVICE)
        return EXT2_INODE_CHAR;
    else if(vfs_type == INODE_BLOCK_DEVICE)
        return EXT2_INODE_BLOCK;
    else if(vfs_type == INODE_SYMBOLIC_LINK)
        return EXT2_INODE_SYMBOLIC_LINK;
    else
        return EXT2_DIRENT_REG;  
}

hret_t ext2_get_inode_block(struct inode *i, uint32_t block_index, uint32_t *result_block_id)
{
    struct ext2_memory_super *ext2_s = i->super->custom_data;
    struct ext2_inode *ext2_i = i->private_data;    
    size_t len_to_read = ext2_s->block_size;    
    uint32_t *indirect1 = NULL;                     // Bloc d'indirection simple
    uint32_t *indirect2 = NULL;                     // Bloc d'indirection double
    uint32_t *indirect3 = NULL;                     // Bloc d'indirection triple
    uint32_t disk_offset;    
    hret_t ret = RET_OK;

    if(EXT2_NEED_INDIRECT1(ext2_s, block_index))
    {
        indirect1 = kmalloc(ext2_s->block_size);
        if(indirect1 == NULL)
            goto no_mem;

        disk_offset = ext2_i->block[12] * ext2_s->block_size;       // Emplacement du bloc d'indirection simple
        
        // On lit le bloc indirect simple
        ret = blockdev_kernel_read(lookup_blockdev(i->super->dev), disk_offset, indirect1, &len_to_read);
        
        if(ret != RET_OK)
            goto free_mem;
    
        if(len_to_read != ext2_s->block_size)
            goto io_error;

        *result_block_id = indirect1[block_index - 12];
    }
    else if(EXT2_NEED_INDIRECT2(ext2_s, block_index))
    {
        indirect1 = kmalloc(ext2_s->block_size);
        indirect2 = kmalloc(ext2_s->block_size);
        block_index -= 13;
        block_index -= (ext2_s->block_size / 4);

        if(indirect1 == NULL || indirect2 == NULL)
            goto no_mem;

        disk_offset = ext2_i->block[13] * ext2_s->block_size;       // Emplacement du bloc d'indirection double
        
        ret = blockdev_kernel_read(lookup_blockdev(i->super->dev), disk_offset, indirect1, &len_to_read);
        
        if(ret != RET_OK)
            goto free_mem;
    
        if(len_to_read != ext2_s->block_size)
            goto io_error;
            
        disk_offset = indirect1[block_index / (ext2_s->block_size / 4)];

        //Lit le block d'indirection double     
        ret = blockdev_kernel_read(lookup_blockdev(i->super->dev), disk_offset, indirect2, &len_to_read);

        if(ret != RET_OK)
            goto free_mem;
    
        if(len_to_read != ext2_s->block_size)
            goto io_error;

        *result_block_id = indirect2[block_index % (ext2_s->block_size / 4)];
    }
    else if(EXT2_NEED_INDIRECT3(ext2_s, block_index))
    {
        panic("Indrection triple non supportée");
    }
    else
    {
        *result_block_id = ext2_i->block[block_index];
    }

    ret = RET_OK;
    goto out;

no_mem:  
    goto free_mem;
io_error:
    ret = -ERR_IO;
free_mem:
out:
    if(indirect1)
        kfree(indirect1);

    if(indirect2)
        kfree(indirect2);

    if(indirect3)
        kfree(indirect3);  
    return ret;
}