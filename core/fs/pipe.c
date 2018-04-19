#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/mm/heap.h>
#include <core/fs/pipe.h>
#include <lib/semaphore.h>
#include <core/process/sleep.h>

static struct open_file_operation pipe_ops;

char pipe_getc(struct pipe_info *pipe_inf)
{
	char c = pipe_inf->pipe_data[pipe_inf->read_off];
	pipe_inf->read_off++;
	if (pipe_inf->read_off == PAGE_SIZE)
		pipe_inf->read_off = 0;

	return c;
}

hret_t pipe_putc(struct pipe_info *pipe_inf, char c)
{
	pipe_inf->pipe_data[pipe_inf->write_off] = c;
	pipe_inf->write_off++;
	if (pipe_inf->write_off == PAGE_SIZE)
		pipe_inf->write_off = 0;

	return RET_OK;
}

hret_t pipe_close_file(struct inode _unused *this_inode, struct file *to_close)
{
    struct pipe_info *pipe_inf = to_close->inode->private_data;

    if(to_close->open_flags & OPEN_WRITE)
        pipe_inf->writers--;
    else
        pipe_inf->readers--;

    // On reveille les processu en attente pour leur indiquer les changements
    if(!pipe_inf->writers)
        wake_up(pipe_inf->standby_readers);
    if(!pipe_inf->readers)
        wake_up(pipe_inf->standby_writers);

    kfree(to_close);            
    return RET_OK;
}

hret_t pipe_destructor(struct inode *to_del)
{
    struct pipe_info *pipe_inf = to_del->private_data;

    if(pipe_inf->readers != 0 || pipe_inf->writers != 0)
        panic("");

    release_semaphore(pipe_inf->atomic_write);
    release_wait_queue(pipe_inf->standby_readers);
    release_wait_queue(pipe_inf->standby_writers);

    PagingUnmap((vaddr_t)pipe_inf->pipe_data);
    FreeVmArea((vaddr_t)pipe_inf->pipe_data);

    kfree(to_del);
    kfree(pipe_inf);
    return RET_OK;
}

/*
 * NOTE: cette fonction ne doit pas être utilisé en dehors de sys_pipe
 */
struct inode *get_pipe_inode(void)
{
    struct pipe_info *pipe_inf = NULL;
    struct inode *pipe_inode = NULL;
    vaddr_t pipe_data = 0;

    pipe_inode = kmalloc(sizeof(struct inode));  
    if(!pipe_inode)
        goto free_mem;

    pipe_inf = kmalloc(sizeof(struct pipe_info));
    if(!pipe_inf)
        goto free_mem;

    pipe_data = get_vm_area(1, 0);
    if(pipe_data == 0)
        goto free_mem;

    PagingAutoMap(pipe_data, 1, VM_MAP_WRITE | VM_MAP_ATOMIC);

    memset(pipe_inode, 0, sizeof(struct inode));
    memset(pipe_inf, 0, sizeof(struct pipe_info));

    pipe_inf->readers = 1;
    pipe_inf->writers = 1;
    pipe_inf->pipe_data = (char *) pipe_data;
    init_wait_queue(&pipe_inf->standby_readers);
    init_wait_queue(&pipe_inf->standby_writers);
    init_semaphore(&pipe_inf->atomic_write, MUTEX);

    pipe_inode->type = INODE_PIPE;
    pipe_inode->memory_count = 2;
    pipe_inode->close_file = pipe_close_file;
    pipe_inode->destructor = pipe_destructor;

    pipe_inode->private_data = pipe_inf;

    return pipe_inode;

free_mem:
    if(pipe_inode)
        kfree(pipe_inode);

    if(pipe_inf)
        kfree(pipe_inf);

    if(pipe_data)
        FreeVmArea(pipe_data);

    return NULL;
}

hret_t duplicate_pipe(struct file *this_file, struct process *for_process, struct file **result)
{
    struct pipe_info *pipe_inf = this_file->inode->private_data;

    (*result) = kmalloc(sizeof(struct file));
	if (!(*result))
		return -ERR_NO_MEM;

	if(this_file->open_flags & OPEN_WRITE)
		pipe_inf->writers++;
	else
		pipe_inf->readers++;

	memcpy((char *)*result, (char *)this_file, sizeof(struct file));
	(*result)->owner = for_process;
	return RET_OK;
}


hret_t make_pipe(int mode, struct file *fd, struct inode *pipe_inode)
{
    if(mode != OPEN_READ && mode != OPEN_WRITE)
        return -ERR_BAD_ARG;

    fd->offset = 0;
    fd->dir_op = NULL;
    fd->char_op = NULL;
    fd->block_op = NULL;

    fd->ref = 1;
    fd->owner = current;
    fd->open_flags = mode;      
    fd->inode = pipe_inode;
    fd->file_op = &pipe_ops;
    fd->duplicate = duplicate_pipe;

    return RET_OK;
}

hret_t pipe_fcntl(struct file _unused *this_file, uint32_t _unused request_id, uint32_t _unused request_arg)
{
    panic("pipe_fcntl");
	return -ERR_NOT_IMPL;
}

hret_t pipe_read(struct file *this_file, char *dst_buf, size_t *len)
{
    struct pipe_info *pipe_inf = this_file->inode->private_data;
    size_t len_readed = 0;

    while(pipe_empty(pipe_inf))                         // S'il n'y a pas de place dans le tube
    {
        wake_up(pipe_inf->standby_writers);             // Réveille les écrivains

        if(!pipe_inf->writers)
        {
            *len = 0;
            return RET_OK;
        }

        sleep_on(pipe_inf->standby_readers);            // Endors le processus
    }

    while(len_readed < *len && !pipe_empty(pipe_inf))
    {
        dst_buf[len_readed] = pipe_getc(pipe_inf);      // On obtient un caractère du tube
        len_readed++;                                   // Incrémente le nombre d'octets écrits
    }
    
    wake_up(pipe_inf->standby_writers);                // On reveille les processus en attente de lecture
    *len = len_readed;
    return RET_OK;
}

hret_t pipe_write(struct file *this_file, char *src_buf, size_t *len)
{
    struct pipe_info *pipe_inf = this_file->inode->private_data;
    size_t len_writed = 0;

    down_semaphore(pipe_inf->atomic_write);             // Un seul processus peut écrire à la fois dans un tube

    if(!pipe_inf->readers)                              // S'il n'y a pas de lecteur
        panic("Broken_pipe (pipe_write)");

    while(len_writed < *len)
    {
        while(pipe_full(pipe_inf))                      // S'il n'y a pas de place dans le tube
        {
            wake_up(pipe_inf->standby_readers);         // Réveille les lecteurs

            if(!pipe_inf->readers)
                panic("Broken_pipe (pipe_write)");

            sleep_on(pipe_inf->standby_writers);        // Endors le processus
        }

        pipe_putc(pipe_inf, src_buf[len_writed]);       // On place un caractère dans le tube

        wake_up(pipe_inf->standby_readers);             // On reveille les processus en attente de lecture
        len_writed++;                                   // Incrémente le nombre d'octets écrits
    }
    
    *len = len_writed;
    up_semaphore(pipe_inf->atomic_write);

    return RET_OK;
}

static struct open_file_operation pipe_ops = {
	NULL,
	&pipe_fcntl,
	&pipe_write,
	&pipe_read
};
