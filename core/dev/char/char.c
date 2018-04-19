#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/fs/file.h>
#include <core/mm/heap.h>
#include <core/dev/char/char.h>

static struct chardev *chardev_list = NULL;
static struct open_file_operation char_file_ops;
static struct open_chardev_ops specific_chardev_ops;

hret_t register_major_chardev(major_t major, struct chardev_ops *ops, void *custom_data)
{
	struct chardev *new_chardev;

	if (lookup_chardev(major) != NULL)
	{
		debugk("[CHARDEV]: Trying to register existing char device !\n");
		return -ERR_BUSY;
	}

	if (ops->open == NULL || ops == NULL)
	{
		debugk("[CHARDEV]: Trying to register device without open function !\n");
		return -ERR_BAD_ARG;
	}

	new_chardev = kmalloc(sizeof(struct chardev));

	if (new_chardev == NULL)
		return -ERR_NO_MEM;

	new_chardev->count = 1;
	new_chardev->ops = ops;
	new_chardev->major = major;
	new_chardev->private_data = custom_data;

	if (list_empty(chardev_list))
		list_singleton(chardev_list, new_chardev);
	else
		list_add_after(chardev_list, new_chardev);

	return RET_OK;
}

hret_t unregister_major_chardev(major_t major)
{
	struct chardev *to_unregistred;
	to_unregistred = lookup_chardev(major);

	if (to_unregistred == NULL)
		return -ERR_BAD_ARG;

	if (to_unregistred->count != 1)
	{
		debugk("[CHARDEV]: trying to unregistred used char device !\n");
		return -ERR_BUSY;
	}

	list_delete(chardev_list, to_unregistred);
	kfree(to_unregistred);
	return RET_OK;
}

struct chardev *lookup_chardev(major_t dev_major)
{
	struct chardev *cur_chardev;
	int nb_chardev;

	if (!list_empty(chardev_list))
	{
		list_foreach(chardev_list, cur_chardev, nb_chardev)
		{
			if (cur_chardev->major == dev_major)
				return cur_chardev;
		}
	}
	return NULL;
}

hret_t duplicate_opened_chardev(struct file *this_file, struct process _unused *for_process, struct file **result)
{
	struct chardev_openfile *to_duplicate = (struct chardev_openfile *)this_file;
	struct chardev *concerned_chardev = to_duplicate->device;
	struct chardev_openfile *duplicated_open_file;
	hret_t ret;

	duplicated_open_file = kmalloc(sizeof(struct chardev_openfile));
	if (duplicated_open_file == NULL)
		return -ERR_NO_MEM;

	concerned_chardev->count++;
	memcpy((char *)duplicated_open_file, (const char *)to_duplicate, sizeof(struct chardev_openfile));

	ret = concerned_chardev->ops->open(this_file->inode, &duplicated_open_file->super, concerned_chardev->private_data);
	if (ret != RET_OK)
	{
		kfree(duplicated_open_file);
		concerned_chardev->count--;
		*result = NULL;

		return ret;
	}

	*result = &duplicated_open_file->super;
	return RET_OK;
}

/*
 * Lorsqu'un fichier est ouvert à partir d'un inode de périphérique caractère
 */
hret_t chardev_open_file(struct inode *this_inode, struct process *owner, flags_t open_flags, struct file **result)
{
	struct chardev *concerned_chardev = lookup_chardev(GET_MAJOR(this_inode->dev));
	struct chardev_openfile *new_open_file;
	hret_t ret;

	if (concerned_chardev == NULL)
		return -ERR_NO_DEV;

	new_open_file = kmalloc(sizeof(struct chardev_openfile));
	if (new_open_file == NULL)
		return -ERR_NO_MEM;

	new_open_file->device = concerned_chardev;
	*result = &new_open_file->super;

	(*result)->char_op = &specific_chardev_ops;
	(*result)->file_op = &char_file_ops;
	(*result)->open_flags = open_flags;
	(*result)->owner = owner;

	ret = concerned_chardev->ops->open(this_inode, *result, concerned_chardev->private_data);
	if (ret != RET_OK)
	{
		kfree(new_open_file);
		concerned_chardev->count--;
		*result = NULL;

		return ret;
	}

	(*result)->duplicate = duplicate_opened_chardev;
	concerned_chardev->count++;

	return RET_OK;
}

/*
* Lorsqu'un fichier est fermé à partir d'un inode de périphérique caractère
*/
hret_t chardev_close_file(struct inode *this_inode, struct file *to_close)
{
	struct chardev *concerned_chardev = lookup_chardev(GET_MAJOR(this_inode->dev));
	hret_t ret = RET_OK;

	if (concerned_chardev == NULL)
		return -ERR_NO_DEV;

	if (concerned_chardev->ops->close != NULL)
		ret = concerned_chardev->ops->close(to_close, concerned_chardev->private_data);
	
	if (ret != RET_OK)
		return ret;

	kfree(to_close);			// Libère un objet chardev_openfile en réalité
	concerned_chardev->count--;

	return RET_OK;
}

/*
* Fonctions appelés lorsque le VFS référence ou libère un inode afin que les périphériques
* caractère soit informés de ce changement
*/
hret_t chardev_ref_new_inode(struct inode *i)
{
	i->open_file = chardev_open_file;
	i->close_file = chardev_close_file;
	// La destruction de l'inode (en mémoire seulement) est géré par le système de fichier

	return RET_OK;
}

hret_t chardev_release_inode(struct inode _unused *i)
{
	return RET_OK;
}

/*
 * Les fonctions communs à tous les fichiers ouverts
 */

hret_t chardev_seek(struct file *this_file, seek_t whence, off_t offset, off_t *result)
{
	struct chardev_openfile *opened_file = (struct chardev_openfile*)this_file;
	struct chardev *chardev_of_this_file = opened_file->device;

	if (chardev_of_this_file->ops->seek != NULL)
		return chardev_of_this_file->ops->seek(this_file, whence, offset, result);

	return -ERR_NO_SYS;
}

hret_t chardev_write(struct file *this_file, char *src_buf, size_t *len)
{
	struct chardev_openfile *opened_file = (struct chardev_openfile*)this_file;
	struct chardev *chardev_of_this_file = opened_file->device;

	if (chardev_of_this_file->ops->write != NULL)
		return chardev_of_this_file->ops->write(this_file, src_buf, len);


	return -ERR_NO_SYS;
}

hret_t chardev_read(struct file *this_file, char *dst_buf, size_t *len)
{
	struct chardev_openfile *opened_file = (struct chardev_openfile*)this_file;
	struct chardev *chardev_of_this_file = opened_file->device;

	if (chardev_of_this_file->ops->read != NULL)
		return chardev_of_this_file->ops->read(this_file, dst_buf, len);

	return -ERR_NO_SYS;
}

hret_t chardev_fcntl(struct file *this_file, uint32_t request_id, uint32_t request_arg)
{
	struct chardev_openfile *opened_file = (struct chardev_openfile*)this_file;
	struct chardev *chardev_of_this_file = opened_file->device;

	if (chardev_of_this_file->ops->fcntl != NULL)
		return chardev_of_this_file->ops->fcntl(this_file, request_id, request_arg);

	return -ERR_NO_SYS;
}

hret_t chardev_ioctl(struct file *this_file, uint32_t request_id, uint32_t request_arg)
{
	struct chardev_openfile *opened_file = (struct chardev_openfile*)this_file;
	struct chardev *chardev_of_this_file = opened_file->device;

	if (chardev_of_this_file->ops->ioctl != NULL)
		return chardev_of_this_file->ops->ioctl(this_file, request_id, request_arg);

	return -ERR_NO_SYS;
}

static struct open_file_operation char_file_ops = {
	&chardev_seek,
	&chardev_fcntl,
	&chardev_write,
	&chardev_read
};

static struct open_chardev_ops specific_chardev_ops = {
	&chardev_ioctl
};
