#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/mm/heap.h>
#include <core/dev/block/block.h>

static uint64_t last_id = 0;
static struct blockdev *blockdev_list = NULL;
static struct open_file_operation block_file_ops;
static struct open_blockdev_ops specific_blockdev_ops;

hret_t init_blockdev(void)
{
    return RET_OK;
}

bool_t blkdev_present(dev_t id)
{
	struct blockdev *bdev = lookup_blockdev(id);
	return (bdev == NULL) ? FALSE : TRUE;
}

/*
 * Cette fonction permet de désenregistrer un périphérique bloc uniquement si ce dernier
 * n'est plus utilisé
 */
hret_t unregister_device(dev_t dev_id)
{
	struct blockdev *blkdev;
	
	blkdev = lookup_blockdev(dev_id);
	if(blkdev == NULL)
		return -ERR_NO_DEV;

	if(blkdev->count > 0)
		return -ERR_BUSY;

	// Supression du périphérique bloc
	if(blkdev->parent != NULL)
		blockdev_unref(blkdev->parent);

	list_delete(blockdev_list, blkdev);
	kfree(blkdev);
	return RET_OK;
}

hret_t blockdev_ref(struct blockdev *dev)
{
	dev->count++;
	return RET_OK;
}

hret_t blockdev_unref(struct blockdev *dev)
{
	if(dev->count <= 0)
		panic("Numéro de référence d'un périphérique bloc invalide");

	dev->count--;
	return RET_OK;
}

hret_t blockdev_ref_by_id(dev_t dev_id)
{
	struct blockdev *blkdev = lookup_blockdev(dev_id);
	if(blkdev == NULL)
		return -ERR_NO_DEV;

	return blockdev_ref(blkdev);
}

struct blockdev *lookup_blockdev(dev_t devid)
{
	struct blockdev *cur_blockdev;
	int nb_blockdev;

	if (!list_empty(blockdev_list))
	{
		list_foreach(blockdev_list, cur_blockdev, nb_blockdev)
		{
			if (cur_blockdev->dev_id == devid)
				return cur_blockdev;
		}
	}
	return NULL;
}

hret_t register_blockdev(dev_t dev_id, size_t block_size, count_t nb_blocks, struct blockdev_ops *ops, void *custom_data)
{
	struct blockdev *new_blockdev;

	new_blockdev = lookup_blockdev(dev_id);
	if(new_blockdev != NULL)
		return -ERR_BUSY;

	if(block_size <= 0)
		return -ERR_BAD_ARG;
	if(nb_blocks <= 0)
		return -ERR_BAD_ARG;
	if(ops == NULL)
		return -ERR_BAD_ARG;

	new_blockdev = kmalloc(sizeof(struct blockdev));
	if(new_blockdev == NULL)
		return -ERR_NO_MEM;

	new_blockdev->count = 1;		
	new_blockdev->ops = ops;
	new_blockdev->dev_id = dev_id;
	new_blockdev->block_num = nb_blocks;
	new_blockdev->block_size = block_size;

	new_blockdev->private_data = custom_data;

	new_blockdev->parent = NULL;
	new_blockdev->first_block = 0;		// Uniquement pour les partitions	

	new_blockdev->id = last_id++;
	if(list_empty(blockdev_list))
		list_singleton(blockdev_list, new_blockdev);
	else	
		list_add_after(blockdev_list, new_blockdev);

	return RET_OK;
}

hret_t register_partition(dev_t dev_id, struct blockdev *parent, count_t number_of_block, off_t first_block_index, void *custom_data)
{
	struct blockdev *new_partiton;
	
	new_partiton = lookup_blockdev(dev_id);						// Vérifie si l'ID fourni est utilisé
	if(new_partiton != NULL)									// Si oui alors on quitte
		return -ERR_BUSY;

	/* Vérifications des paramètres */
	if(parent == NULL)
		return -ERR_BAD_ARG;
	if(number_of_block <= 0)
		return -ERR_BAD_ARG;

	new_partiton = kmalloc(sizeof(struct blockdev));
	if(new_partiton == NULL)
		return -ERR_NO_MEM;
		
	new_partiton->count = 1;		
	new_partiton->dev_id = dev_id;
	new_partiton->ops = parent->ops;
	new_partiton->block_num = number_of_block;
	new_partiton->block_size = parent->block_size;

	new_partiton->id = last_id++;							// Identification 100% unique
	new_partiton->private_data = custom_data;

	new_partiton->parent = parent;
	new_partiton->first_block = parent->first_block + first_block_index;

	blockdev_ref(parent);	
	if(list_empty(blockdev_list))
		list_singleton(blockdev_list, new_partiton);
	else	
		list_add_after(blockdev_list, new_partiton);

	return RET_OK;
}

static hret_t blkdev_generic_read(struct blockdev *blkdev, off_t offset, void *buf, size_t *len)
{
	uint32_t byte_to_copy = 0;	
	uint32_t byte_readed = 0;
	uint32_t block;
	char *buf_ptr;
	char *block_buf;

	hret_t ret = RET_OK;

	if(blkdev == NULL)
		return -ERR_BAD_ARG;

	block_buf = kmalloc(blkdev->block_size);	
	blockdev_ref(blkdev);
	buf_ptr = buf;
	
	while(byte_readed < *len)
	{
		block = offset / blkdev->block_size;
		if(block >= blkdev->block_num)
			break;

		block += blkdev->first_block;			// Transpose le bloc dans le périphérique
		ret = blkdev->ops->read_block(blkdev->private_data, block_buf, block);

		if(ret != RET_OK)
			break;

		byte_to_copy = ((*len - byte_readed) >= (blkdev->block_size - (offset % blkdev->block_size))) ? blkdev->block_size - (offset % blkdev->block_size) : *len - byte_readed;
		memcpy(buf_ptr, block_buf + (offset % blkdev->block_size), byte_to_copy);

		offset += byte_to_copy;
		buf_ptr += byte_to_copy;
		byte_readed += byte_to_copy;
	}

	kfree(block_buf);
	*len = byte_readed;
	blockdev_unref(blkdev);	
	return ret;
}

static hret_t blkdev_generic_write(struct blockdev *blkdev, off_t offset, void *buf, size_t *len)
{
	uint32_t byte_writed = 0;
	uint32_t byte_to_copy;
	uint32_t block;
	char *buf_ptr;
	char *block_buf;

	hret_t ret = RET_OK;
	
	if(blkdev->ops->write_block == NULL)
		return -ERR_NO_SYS;

	if(blkdev == NULL)
		return -ERR_BAD_ARG;

	block_buf = kmalloc(blkdev->block_size);
	blockdev_ref(blkdev);
	buf_ptr = buf;
	
	while(byte_writed < *len)
	{
		block = offset / blkdev->block_size;
		if(block >= blkdev->block_num)
			break;

		block += blkdev->first_block;			// Transpose le bloc dans le périphérique

		if((offset % blkdev->block_size) != 0)	// Ecriture au milieu d'un bloc
		{
			ret = blkdev->ops->read_block(blkdev->private_data, block_buf, block);	
			memcpy(block_buf + (offset % blkdev->block_size), buf_ptr, 
				   ((*len - byte_writed) >= (blkdev->block_size - (offset % blkdev->block_size))) ? blkdev->block_size - (offset % blkdev->block_size) : *len - byte_writed);	
		}
		else if((*len - byte_writed) < blkdev->block_size)		// Ecriture partiel d'un bloc
		{
			ret = blkdev->ops->read_block(blkdev->private_data, block_buf, block);	
			memcpy(block_buf, buf_ptr, *len - byte_writed);	
		}
		else
		{
			memcpy(block_buf, buf_ptr, blkdev->block_size);		// Ecriture totale d'un block
		}

		ret = blkdev->ops->write_block(blkdev->private_data, block_buf, block);
		if(ret != RET_OK)
			break;
			
		byte_to_copy = ((*len - byte_writed) >= (blkdev->block_size - (offset % blkdev->block_size))) ? blkdev->block_size - (offset % blkdev->block_size) : *len - byte_writed;
			
		offset += byte_to_copy;
		buf_ptr += byte_to_copy;
		byte_writed += byte_to_copy;
	}

	*len = byte_writed;
	blockdev_unref(blkdev);	
	kfree(block_buf);
	return ret;
}

hret_t blockdev_kernel_read(struct blockdev *to_read, off_t offset, void *dst_buf, size_t *len)
{
	return blkdev_generic_read(to_read, offset, dst_buf, len);
}

hret_t blockdev_kernel_write(struct blockdev *to_write, off_t offset, void *src_buf, size_t *len)
{
	return blkdev_generic_write(to_write, offset, src_buf, len);
}

hret_t duplicate_opened_blockdev(struct file *this_file, struct process *for_process, struct file **result)
{
	*result = kmalloc(sizeof(struct file));
	if(*result == NULL)
		goto no_mem;

	memcpy(*result, this_file, sizeof(struct file));
	(*result)->owner = for_process;
	return RET_OK;

no_mem:
	*result = NULL;
	return -ERR_NO_MEM;
}

hret_t blockdev_open_file(struct inode *this_inode, struct process *owner, flags_t open_flags, struct file **result)
{
	struct blockdev *blkdev = lookup_blockdev(this_inode->dev);

	if(blkdev == NULL)
		return -ERR_NO_DEV;

	*result = kmalloc(sizeof(struct file));
	if(*result == NULL)
		return -ERR_NO_MEM;

	(*result)->ref = 0;	
	(*result)->offset = 0;
	(*result)->owner = owner;
	(*result)->generation = 0;	
	(*result)->inode = this_inode;	
	(*result)->custom_data = NULL;
	(*result)->open_flags = open_flags;
	(*result)->file_op = &block_file_ops;	
	(*result)->block_op = &specific_blockdev_ops;	
	(*result)->duplicate = &duplicate_opened_blockdev;
	
	return RET_OK;
}

hret_t blockdev_close_file(struct inode _unused *this_inode, struct file *to_close)
{
	kfree(to_close);
	return RET_OK;
}

hret_t blockdev_ref_new_inode(struct inode *i)
{
	struct blockdev *blkdev = lookup_blockdev(i->dev);
	if(blkdev == NULL)
		return -ERR_NO_DEV;

	blockdev_ref(blkdev);

	i->open_file = blockdev_open_file;
	i->close_file = blockdev_close_file;
	// La destruction de l'inode (en mémoire seulement) est géré par le système de fichier

	return RET_OK;
}

hret_t blockdev_release_inode(struct inode *i)
{
	struct blockdev *blkdev = lookup_blockdev(i->dev);
	if(blkdev == NULL)
		return -ERR_NO_DEV;

	blockdev_unref(blkdev);
	return RET_OK;	
}

/*
 * Les fonction ci dessous servent de wrapper pour les fonction "file" du VFS et qui doivent
 * être implémentées (ou du moins retourner un code d'erreur si la fonction n'est pas supportée)
 */

hret_t blockdev_seek(struct file *this_file, seek_t whence, off_t offset, off_t *result)
{
	struct blockdev *blkdev = lookup_blockdev(this_file->inode->dev);
	off_t starting_offset;

	if(blkdev == NULL)
		return -ERR_NO_DEV;

	switch (whence)
	{
		case SEEK_CUR:
			starting_offset = this_file->offset;
			break;

		case SEEK_END:
			starting_offset = blkdev->block_size * blkdev->block_num;
			break;

		case SEEK_SET:
			starting_offset = 0;
			break;

		default:
			return -ERR_BAD_ARG;
			break;
	}

	if (offset < -starting_offset)			// Si l'offset se positionne avant le début du fichier (le résultat sera négatif)
		return -ERR_BAD_ARG;				// On retourne une erreur

	if ((starting_offset + offset) > (blkdev->block_size * blkdev->block_num))
		return -ERR_BAD_ARG;				// On retourne une erreur
	
	this_file->offset = starting_offset + offset;
	*result = this_file->offset;
	return RET_OK;
}

hret_t blockdev_write(struct file *this_file, char *src_buf, size_t *len)
{
	struct blockdev *blkdev = lookup_blockdev(this_file->inode->dev);
	hret_t ret;

	if(blkdev == NULL)
		return -ERR_NO_DEV;

	ret = blkdev_generic_write(blkdev, this_file->offset, src_buf, len);
	this_file->offset += *len;

	return ret;
}

hret_t blockdev_read(struct file *this_file, char *dst_buf, size_t *len)
{
	struct blockdev *blkdev = lookup_blockdev(this_file->inode->dev);
	hret_t ret;

	if(blkdev == NULL)
		return -ERR_NO_DEV;

	ret = blkdev_generic_read(blkdev, this_file->offset, dst_buf, len);
	this_file->offset += *len;

	return ret;
}

hret_t blockdev_fcntl(struct file _unused *this_file, uint32_t _unused request_id, uint32_t _unused request_arg)
{
	return -ERR_NO_SYS;
}

hret_t blockdev_ioctl(struct file *this_file, uint32_t request_id, uint32_t request_arg)
{
	struct blockdev *blkdev = lookup_blockdev(this_file->inode->dev);
	if(blkdev == NULL)
		return -ERR_NO_DEV;
	
	if(blkdev->ops->ioctl != NULL)
		return blkdev->ops->ioctl(blkdev->private_data, request_id, request_arg);

	return -ERR_NO_SYS;
}

static struct open_file_operation block_file_ops = {
	&blockdev_seek,
	&blockdev_fcntl,
	&blockdev_write,
	&blockdev_read
};

static struct open_blockdev_ops specific_blockdev_ops = {
	&blockdev_ioctl
};
