#include <lib/stdio.h>
#include <lib/stdarg.h>
#include <lib/stdlib.h>
#include <core/fs/vfs.h>
#include <core/mm/heap.h>
#include <core/fs/super.h>
#include <lib/vsnprintf.h>
#include <core/fs/ext2/ext2.h>
#include <core/dev/block/block.h>

static struct file_system ext2_fs;
struct super_operation ext2_super_op;			        // Opérations du super_block
struct file_system_operation ext2_fs_op;			    // Opérations du système de fichier

struct inode_operation ext2_inode_op;			        // Opération pour les fichiers
struct inode_dir_operation ext2_dir_op;			        // Opérations pour les répertoires

struct open_dir_operation ext2_open_dir_op;		        // Opération pour un répertoire ouvert
struct open_file_operation ext2_open_file_op;	        // Opération pour un fichier ouvert

void RegisterExt2(void)
{
    ext2_fs.name = "ext2";
    ext2_fs.fs_op = &ext2_fs_op;

    ext2_fs_op.mount = ext2_mount;
    ext2_fs_op.umount = ext2_umount;

    ext2_super_op.fetch_inode = ext2_fetch_inode;    
    ext2_super_op.allocate_inode = ext2_allocate_inode;

    ext2_inode_op.sync = ext2_sync;
    ext2_inode_op.stat = ext2_stat;
    ext2_inode_op.truncate = ext2_truncate;

    ext2_dir_op.link = ext2_link;
    ext2_dir_op.lookup = ext2_lookup;
    ext2_dir_op.unlink = ext2_unlink;

    ext2_open_dir_op.readdir = ext2_readdir;

    ext2_open_file_op.fnctl = NULL;             // A implémenter lorsque cet appel système sera fait
    ext2_open_file_op.read = ext2_read;
    ext2_open_file_op.seek = ext2_seek;
    ext2_open_file_op.write = ext2_write;

    register_fs(&ext2_fs);
}

#ifdef DEBUG_MODE
void ext2_debugk(char *format, ...)
#else
void ext2_debugk(char _unused *format, ...)
#endif
{
#ifdef DEBUG_MODE
	char buffer[256];
	va_list arg;

	va_start(arg, format);
	vsnprintf(buffer, 256, format, arg);
	va_end(arg);

	debugk("[EXT2]: %s", buffer);
#endif
}

/*
 * Permet de lire un descripteur de groupe situé sur le disque
 */
hret_t ext2_read_group_descriptor(struct super_block *super, uint32_t index, struct ext2_group_descriptor **result)
{
    struct ext2_memory_super *ext2_super = super->custom_data;
    size_t len_to_read = sizeof(struct ext2_group_descriptor);    
    uint32_t offset = 0;
    hret_t ret = RET_OK;

    if(index > ext2_super->nb_groups)
        return -ERR_BAD_ARG;

    (*result) = kmalloc(len_to_read);
    if((*result) == NULL)
        return -ERR_NO_MEM;

    // Emplacement du descripteur de groupe
    offset = (ext2_super->block_size == 1024) ? 2048 : ext2_super->block_size;
    offset += index * sizeof(struct ext2_group_descriptor);

    ret = blockdev_kernel_read(lookup_blockdev(super->dev), offset, *result, &len_to_read);
    if(ret != RET_OK)
        goto free_mem;

    if(len_to_read != sizeof(struct ext2_group_descriptor))
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
 * Cette fonction permet d'écrire un descripteur de groupe sur le disque
 */
hret_t ext2_write_group_descriptor(struct super_block *super, uint32_t index, struct ext2_group_descriptor *gdesc)
{
    struct ext2_memory_super *ext2_super = super->custom_data;
    size_t len_to_write = sizeof(struct ext2_group_descriptor);    
    uint32_t offset = 0;
    hret_t ret = RET_OK;

    if(index > ext2_super->nb_groups)
        return -ERR_BAD_ARG;

    // Emplacement du descripteur de groupe
    offset = (ext2_super->block_size == 1024) ? 2048 : ext2_super->block_size;
    offset += index * sizeof(struct ext2_group_descriptor);

    ret = blockdev_kernel_write(lookup_blockdev(super->dev), offset, gdesc, &len_to_write);
    if(ret != RET_OK)
        goto out;

    if(len_to_write != sizeof(struct ext2_group_descriptor))
        goto io_error;

    goto out;

io_error:
    ret = -ERR_IO;
out:
    return ret;
}

/*
 * Permet d'obtenir un inode libre et de marquer celui ci comme étant utilisé. Retourne -ERR_NO_ENTRY si
 * aucun inode n'est disponible sur le système de fichier, ou le numéro d'inode obtenu
 */
hret_t ext2_get_free_inode(struct super_block *super, uint32_t *result_inode_id)
{
    struct ext2_memory_super *ext2_s = (struct ext2_memory_super *)super->custom_data;
    struct ext2_group_descriptor *gdesc = NULL;
    uint8_t *buffer = kmalloc(ext2_s->block_size);                 // Le bitmap occupe toujours 1 bloc
    size_t len_to_write = ext2_s->block_size;                   // Pour la fonction blockdev_kernel_write        
    size_t len_to_read = ext2_s->block_size;                    // Pour la fonction blockdev_kernel_read  
    hret_t ret;

    *result_inode_id = 0;

    for(unsigned int i = 0; i < ext2_s->nb_groups; i++)
    {
        if(gdesc)
            kfree(gdesc);

        ret = ext2_read_group_descriptor(super, i, &gdesc);
        if(ret != RET_OK)
            goto out;

        // Vérifie si il y a des inodes libres dans ce descripteur de groupe
        if(gdesc->free_inode_count == 0)
            continue;

        //Cherche un inode libre
        ret = blockdev_kernel_read(lookup_blockdev(super->dev), gdesc->inode_bitmap * ext2_s->block_size, buffer, &len_to_read);
        if(ret != RET_OK)
            goto out;

        if(len_to_read != ext2_s->block_size)
            goto out;

        for(unsigned int j = 0; j < ext2_s->block_size; j++)
        {
            if(buffer[j] == 0xFF)
                continue;
                
            // Ici, on a trouvé un inode libre
            for(unsigned int bit = 0; bit < 8; bit++)
            {
                if((buffer[j] & (1 << bit)) == 0)
                {

                    // On actualise le buffer
                    buffer[j] |= (1 << bit);
                    ret = blockdev_kernel_write(lookup_blockdev(super->dev), gdesc->inode_bitmap * ext2_s->block_size, buffer, &len_to_write);                    
                    if(ret != RET_OK)
                        goto out;
            
                    if(len_to_write != ext2_s->block_size)
                        goto out;

                    // On actualise le descripteur de groupe
                    gdesc->free_inode_count--;
                    ret = ext2_write_group_descriptor(super, i, gdesc);
                    if(ret != RET_OK)
                        goto out;

                    // On acualise le super block
                    ext2_s->super.free_inodes_count--;
                    ext2_s->is_dirty = TRUE;

                    *result_inode_id = (i * ext2_s->block_size) + (j * 8) + bit + 1; 
                    debugk("Getting inode %u\n", *result_inode_id);             

                    kfree(buffer);
                    return RET_OK;
                }
            }
        }
    }

out:
    *result_inode_id = 0;  
    kfree(buffer);
    return -ERR_NO_ENTRY;
}

/*
 * Permet d'obtenir un block libre et de marquer celui ci comme étant utilisé. Retourne -ERR_NO_ENTRY si
 * aucun bloc n'est disponible sur le système de fichier, ou le numéro de bloc obtenu en fonction de la
 * taille des bloc du système de fichier ext2
 */
hret_t ext2_get_free_block(struct super_block *super, uint32_t *result_block_id)
{
    struct ext2_memory_super *ext2_s = (struct ext2_memory_super *) super->custom_data;
    struct ext2_group_descriptor *gdesc = NULL;
    uint8_t *buffer = kmalloc(ext2_s->block_size);                 // Le bitmap occupe toujours 1 bloc
    size_t len_to_write = ext2_s->block_size;                   // Pour la fonction blockdev_kernel_write        
    size_t len_to_read = ext2_s->block_size;                    // Pour la fonction blockdev_kernel_read  
    hret_t ret;

    *result_block_id = 0;

    for(unsigned int i = 0; i < ext2_s->nb_groups; i++)
    {
        if(gdesc)
            kfree(gdesc);

        ret = ext2_read_group_descriptor(super, i, &gdesc);
        if(ret != RET_OK)
            goto out;

        // Vérifie si il y a des blocks libres dans ce descripteur de groupe
        if(gdesc->free_blocks_count == 0)
            continue;

        //Cherche un block libre
        ret = blockdev_kernel_read(lookup_blockdev(super->dev), gdesc->block_bitmap * ext2_s->block_size, buffer, &len_to_read);
        if(ret != RET_OK)
            goto out;

        if(len_to_read != ext2_s->block_size)
            goto out;

        for(unsigned int j = 0; j < ext2_s->block_size; j++)
        {
            if(buffer[j] == 0xFF)
                continue;
                
            // Ici, on a trouvé un block libre
            for(unsigned int bit = 0; bit < 8; bit++)
            {
                if((buffer[j] & (1 << bit)) == 0)
                {
                    // On actualise le buffer
                    buffer[j] |= (1 << bit);
                    ret = blockdev_kernel_write(lookup_blockdev(super->dev), gdesc->block_bitmap * ext2_s->block_size, buffer, &len_to_write);                    
                    if(ret != RET_OK)
                        goto out;
            
                    if(len_to_write != ext2_s->block_size)
                        goto out;

                    // On actualise le descripteur de groupe
                    gdesc->free_blocks_count--;
                    ret = ext2_write_group_descriptor(super, i, gdesc);
                    if(ret != RET_OK)
                        goto out;

                    // On acualise le super block
                    ext2_s->super.free_block_count--;
                    ext2_s->is_dirty = TRUE;        

                    // Indique le bloc comme alloué et libère le buffer
                    *result_block_id = (i * ext2_s->block_size) + (j * 8) + bit + 1;     

                    kfree(buffer);
                    return RET_OK;
                }
            }
        }
    }

out:
    kfree(buffer);
    return -ERR_NO_ENTRY;
}

hret_t ext2_release_block(struct super_block *super, uint32_t block_id)
{
    struct ext2_memory_super *ext2_s = (struct ext2_memory_super *) super->custom_data;
    struct ext2_group_descriptor *gdesc = NULL;
    uint8_t *buffer = kmalloc(ext2_s->block_size);              // Le bitmap occupe toujours 1 bloc
    size_t len_to_write = ext2_s->block_size;                   // Pour la fonction blockdev_kernel_write        
    size_t len_to_read = ext2_s->block_size;                    // Pour la fonction blockdev_kernel_read  
    uint32_t *bitmap_ptr = (uint32_t *)buffer;                  // Pointeur 32 bits sur le buffer
    uint32_t group_offset = block_id % ext2_s->block_size;    
    uint32_t group_num = block_id / ext2_s->block_size;
    hret_t ret;

    ret = ext2_read_group_descriptor(super, group_num, &gdesc);
    if(ret != RET_OK)
        goto out;
   
    
    ret = blockdev_kernel_read(lookup_blockdev(super->dev), gdesc->block_bitmap * ext2_s->block_size, buffer, &len_to_read);
    if(ret != RET_OK)
        goto out;

    if(len_to_read != ext2_s->block_size)
        goto out;
    
    bitmap_ptr = (uint32_t *)buffer;
    bitmap_ptr[group_offset / 4] &= ~(1 << ((group_offset / 4) % 32));
    ret = blockdev_kernel_write(lookup_blockdev(super->dev), gdesc->block_bitmap * ext2_s->block_size, buffer, &len_to_write);                    
    if(ret != RET_OK)
        goto out;

    if(len_to_write != ext2_s->block_size)
        goto out;

    // On actualise le descripteur de groupe
    gdesc->free_blocks_count++;
    ret = ext2_write_group_descriptor(super, group_num, gdesc);
    if(ret != RET_OK)
        goto out;

    // On acualise le super block
    ext2_s->super.free_block_count++;
    ext2_s->is_dirty = TRUE;
                

    // Libère le buffer
    kfree(buffer);
    return RET_OK;
            
out:
    kfree(buffer);
    return -ERR_NO_ENTRY;
}


hret_t ext2_release_inode(struct super_block *super, uint32_t inode_id, struct ext2_inode _unused *ext2_i)
{
    struct ext2_memory_super *ext2_s = (struct ext2_memory_super *) super->custom_data;
    struct ext2_group_descriptor *gdesc = NULL;
    char *buffer = kmalloc(ext2_s->block_size);                 // Le bitmap occupe toujours 1 bloc
    size_t len_to_write = ext2_s->block_size;                   // Pour la fonction blockdev_kernel_write        
    size_t len_to_read = ext2_s->block_size;                    // Pour la fonction blockdev_kernel_read  
    uint32_t *bitmap_ptr = (uint32_t *)buffer;                  // Pointeur 32 bits sur le buffer
    uint32_t group_offset = inode_id % ext2_s->block_size;    
    uint32_t group_num = inode_id / ext2_s->block_size;
    hret_t ret;

    ret = ext2_read_group_descriptor(super, group_num, &gdesc);
    if(ret != RET_OK)
        goto out;
   
    
    ret = blockdev_kernel_read(lookup_blockdev(super->dev), gdesc->block_bitmap * ext2_s->block_size, buffer, &len_to_read);
    if(ret != RET_OK)
        goto out;

    if(len_to_read != ext2_s->block_size)
        goto out;
    
    bitmap_ptr = (uint32_t *)buffer;
    bitmap_ptr[group_offset / 4] &= ~(1 << ((group_offset / 4) % 32));
    ret = blockdev_kernel_write(lookup_blockdev(super->dev), gdesc->block_bitmap * ext2_s->block_size, buffer, &len_to_write);                    
    if(ret != RET_OK)
        goto out;

    if(len_to_write != ext2_s->block_size)
        goto out;

    // On actualise le descripteur de groupe
    gdesc->free_inode_count++;
    ret = ext2_write_group_descriptor(super, group_num, gdesc);
    if(ret != RET_OK)
        goto out;

    // On acualise le super block
    ext2_s->super.free_inodes_count++;
    ext2_s->is_dirty = TRUE;

    // Libère le buffer
    kfree(buffer);
    return RET_OK;
            
out:
    kfree(buffer);
    return -ERR_NO_ENTRY;
}
