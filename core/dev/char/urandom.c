#include <core/time/time.h>
#include <core/dev/char/char.h>
#include <core/dev/char/urandom.h>

static struct chardev_ops urandom_ops;

hret_t init_urandom(void)
{
    register_major_chardev(URANDOM_MAJOR, &urandom_ops, NULL);
    return RET_OK;
}

hret_t urandom_open(struct inode _unused *opened_inode, struct file *opened_file, void _unused *private_data)
{
	opened_file->custom_data = (void *)get_current_unix_time();
	return RET_OK;
}

hret_t urandom_read(struct file *open_file, char *src_buf, size_t *len)
{
    for(size_t i = 0; i < *len; i++)
    {
        open_file->custom_data = (void *) ((uint32_t)open_file->custom_data * 1103515245 + 12345);
        src_buf[i] = (unsigned char)(((uint32_t)open_file->custom_data / 65536) % 0xFF);
    }

	return RET_OK;
}

static struct chardev_ops urandom_ops = {
	&urandom_open,
	NULL,
	NULL,
	&urandom_read,
	NULL,
	NULL,
	NULL
};
