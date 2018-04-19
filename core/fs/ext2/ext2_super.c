#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/mm/heap.h>
#include <core/fs/ext2/ext2.h>
#include <core/dev/block/block.h>

/*
 * Cette fonction permet au VFS de monter un système de fichier ext2
 */
hret_t ext2_mount(struct file_system *this_fs, dev_t device, struct super_block **result_super)
{
    struct ext2_memory_super *ext2_super;
    struct inode *root_inode;
    size_t len_to_read; 
    hret_t ret;

    (*result_super) = NULL;
    
    // Vérifications des paramètres
    if(device == 0 || this_fs == NULL)
        return -ERR_BAD_ARG;

    // Allocation de la mémoire nécessaire
    ext2_super = kmalloc(sizeof(struct ext2_memory_super));
    (*result_super) = kmalloc(sizeof(struct super_block));
    
    // Vérifications des allocations
    if(ext2_super == NULL || (*result_super) == NULL)
        return -ERR_NO_MEM;

    // Mise à zéro des structures
    memset(ext2_super, 0, sizeof(struct ext2_memory_super));
    memset(*result_super, 0, sizeof(struct super_block));
   
    len_to_read = sizeof(struct ext2_disk_super);
    ret = blockdev_kernel_read(lookup_blockdev(device), 1024, &ext2_super->super, &len_to_read);

    if(ret != RET_OK || len_to_read != sizeof(struct ext2_disk_super))
        return -ERR_IO;
            
    /* Calcul le nombre de groupe présent dans le système de fichier*/
    int a = ext2_super->super.block_count /  ext2_super->super.blocks_per_group + (( ext2_super->super.block_count %  ext2_super->super.blocks_per_group) ? 1 : 0);
    int b = ext2_super->super.inodes_count /  ext2_super->super.inodes_per_group + (( ext2_super->super.inodes_count %  ext2_super->super.inodes_per_group) ? 1 : 0);
    
    ext2_super->is_dirty = TRUE;                // Il faurait penser à modifier certaines données du super (comme le dernière montage)
    ext2_super->nb_groups = (a > b) ? a : b;
    ext2_super->block_size = 1024 << ext2_super->super.log2_block_size;

    // On initialise le super block du VFS
    (*result_super)->fs = this_fs;
    (*result_super)->dev = device;
    (*result_super)->root = NULL;
    (*result_super)->s_op = &ext2_super_op;    
    (*result_super)->custom_data = ext2_super;

    // Obtient l'inode root
    ret = ext2_get_vfs_inode(*result_super, EXT2_INODE_ROOT, &root_inode);
    if(ret != RET_OK)
        panic("Mouting ext2 failed");

    // Enregistre l'inode root
    register_root_inode(*result_super, root_inode);

    ext2_debugk("Mounting ext2 fs on device %08x succes\n", device);
    return RET_OK;
}

hret_t ext2_umount(struct file_system _unused *this_fs, struct super_block _unused *to_umount)
{
    return -ERR_NOT_IMPL;
}

hret_t ext2_write_super(struct ext2_memory_super *ext2_s, uint32_t dev)
{
    size_t len_to_write = sizeof(struct ext2_disk_super);
    hret_t ret;

    ret = blockdev_kernel_write(lookup_blockdev(dev), 1024, &ext2_s->super, &len_to_write);  

    if(ret != RET_OK)
        return ret;

    if(len_to_write != sizeof(struct ext2_disk_super))
        return -ERR_IO;

    return RET_OK;
}
