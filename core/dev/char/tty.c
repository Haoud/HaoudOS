#include <lib/list.h>
#include <core/mm/heap.h>
#include <driver/console.h>
#include <driver/vga/text.h>
#include <core/dev/char/tty.h>
#include <core/dev/char/char.h>
#include <core/process/sleep.h>
#include <driver/keyboard/keyboard.h>

static struct tty_device *tty_list;
static struct chardev_ops tty_ops;

hret_t tty_init(void)
{
	struct tty_device *first_tty;
	hret_t ret;

	list_init(tty_list);
	ret = register_major_chardev(TTY_MAJOR, &tty_ops, NULL);

	if (ret != RET_OK)
		return ret;

	ret = tty_create(0, console_write, &first_tty);

	if (ret != RET_OK)
		return ret;

	kbd_set_current_tty(first_tty);
	return RET_OK;
}


hret_t tty_add_char(struct tty_device *tty, char c)
{
	if (!tty_buffer_full(tty->raw))
		tty_buffer_putc(tty->raw, c);

	if (test_bit(tty->params, TTY_ECHO) && test_bit(tty->params, TTY_CANONIC_MODE))
		tty->write(tty, c);
	
	if (c == '\n' && test_bit(tty->params, TTY_CANONIC_MODE))
	{
		// On déplace les caractères du buffer brut au buffer structuré
		while (!tty_buffer_empty(tty->raw))
		{
			if (tty_buffer_full(tty->cooked))
				break;

			tty_buffer_putc(tty->cooked, tty_buffer_getc(tty->raw));
		}

		wake_up(tty->wait_on_enter);
	}

	return RET_OK;
}

hret_t tty_add_chars(struct tty_device *tty, char *s)
{
	//down
	while (*s)
	{
		tty_add_char(tty, *s);
		s++;
	}
	//up
	wake_up(tty->wait_on_buffer);
	return RET_OK;
}

hret_t tty_remove(struct tty_device *tty)
{
	if (tty == NULL)
		return -ERR_BAD_ARG;

	// Try down

	if (tty->count != 0)
		return -ERR_BUSY;

	list_delete(tty_list, tty);					// Supprime le terminal de la liste
	kfree(tty);									// Libère la structure
	 
	return RET_OK;
}

hret_t tty_create(minor_t tty_minor, hret_t(*write)(const struct tty_device *tty, char c), struct tty_device **tty_result)
{
	struct tty_device *new_tty;

	if (lookup_tty(tty_minor) != NULL)
		return -ERR_BUSY;

	if (write == NULL)
		return -ERR_BAD_ARG;

	new_tty = kmalloc(sizeof(struct tty_device));
	if (new_tty == NULL)
		return -ERR_NO_MEM;

	new_tty->raw = kmalloc(sizeof(struct tty_buffer));
	new_tty->cooked = kmalloc(sizeof(struct tty_buffer));

	if (!new_tty->raw || !new_tty->cooked)
	{
		kfree(new_tty->raw);
		kfree(new_tty->cooked);
		kfree(new_tty);
		return -ERR_NO_MEM;
	}

	new_tty->count = 0;
	new_tty->timeout = 0;
	new_tty->write = write;
	new_tty->minor = tty_minor;
	new_tty->params = TTY_DEFAULT_PARAMS;

	init_tty_buffer(new_tty->raw);
	init_tty_buffer(new_tty->cooked);

	init_wait_queue(&new_tty->wait_on_buffer);
	init_wait_queue(&new_tty->wait_on_enter);

	if (list_empty(tty_list))
		list_singleton(tty_list, new_tty);
	else
		list_add_after(tty_list, new_tty);

	(*tty_result) = new_tty;
	return RET_OK;
}

struct tty_device *lookup_tty(minor_t tty_minor)
{
	struct tty_device *tty;
	int nb_tty;

	if (!list_empty(tty_list))
	{
		list_foreach(tty_list, tty, nb_tty)
		{
			if (tty->minor == tty_minor)
				return tty;
		}
	}

	return NULL;
}

hret_t tty_open(struct inode *opened_inode, struct file *opened_file, void _unused *private_data)
{
	struct tty_device *tty = lookup_tty(GET_MINOR(opened_inode->dev));

	if (tty == NULL)
		return  ERR_NO_ENTRY;

	opened_file->custom_data = tty;
	tty->count++;
	return RET_OK;
}

hret_t tty_read(struct file *open_file, char *src_buf, size_t *len)
{
	struct tty_device *concerned_tty = (struct tty_device *) open_file->custom_data;
	count_t count = 0;
	char c;
	if (concerned_tty->params & TTY_CANONIC_MODE)
	{
		while (1)
		{
			// down
			if (tty_buffer_empty(concerned_tty->cooked))
			{
				// up
				sleep_on(concerned_tty->wait_on_enter);
				continue;
			}

			c = tty_buffer_getc(concerned_tty->cooked);

			*src_buf = c;
			src_buf++;
			count++;

			//up
			if (*len == count || c == '\n')
				break;
		}
	}
	else
	{
		while (1)
		{
			// down
			if (tty_buffer_empty(concerned_tty->raw))
			{
			// up

				if(concerned_tty->timeout != 0)
				{
					sleep_on_timeout(concerned_tty->wait_on_buffer, concerned_tty->timeout * 100);
					if(tty_buffer_empty(concerned_tty->raw))
						break;
				}
				else
				{
					sleep_on(concerned_tty->wait_on_buffer);
				}

				continue;
			}

			c = tty_buffer_getc(concerned_tty->raw);

			if (test_bit(concerned_tty->params, TTY_ECHO))
				concerned_tty->write(concerned_tty, c);

			*src_buf = c;
			src_buf++;
			count++;

			//up
			if (*len == count)
				break;
		}
	}

	*len = count;
	return RET_OK;
}

hret_t tty_write(struct file *open_file, char *src_buf, size_t *len)
{
	struct tty_device *concerned_tty = (struct tty_device *) open_file->custom_data;

	for (unsigned int i = 0; i < *len; i++)
	{
		concerned_tty->write(concerned_tty, *src_buf);
		src_buf++;
	}

	return RET_OK;
}

hret_t tty_ioctl(struct file *open_file, uint32_t request_id, uint32_t request_arg)
{
	struct tty_device *tty = (struct tty_device *) open_file->custom_data;
	switch (request_id)
	{
		case TTY_SET_FLAG:
			set_bits(tty->params, request_arg);
			break;

		case TTY_CLEAR_FLAG:
			clear_bits(tty->params, request_arg);
			break;
			
		case TTY_RESTORE_DEFAULT:
			set_bits(tty->params, TTY_ECHO);
			set_bits(tty->params, TTY_CANONIC_MODE);
			break;
		
		case TTY_GET_FLAG:
			return test_bit(tty->params, request_arg);

		case TTY_SET_TIMEOUT:
			break;

		default:
			return -ERR_NOT_IMPL;
			break;
	}

	return RET_OK;
}

static struct chardev_ops tty_ops = {
	&tty_open,
	NULL,
	NULL,
	&tty_read,
	&tty_write,
	NULL,
	&tty_ioctl
};

char tty_buffer_getc(struct tty_buffer *tty_buf)
{
	char c = tty_buf->buffer[tty_buf->read_off];
	tty_buf->read_off++;
	if (tty_buf->read_off == TTY_BUFFER_SIZE)
		tty_buf->read_off = 0;

	return c;
}

hret_t tty_buffer_putc(struct tty_buffer *tty_buf, char c)
{
	tty_buf->buffer[tty_buf->write_off] = c;
	tty_buf->write_off++;
	if (tty_buf->write_off == TTY_BUFFER_SIZE)
		tty_buf->write_off = 0;

	return RET_OK;
}