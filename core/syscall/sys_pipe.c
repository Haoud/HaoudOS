#include <lib/stdio.h>
#include <core/fs/pipe.h>
#include <core/mm/heap.h>
#include <core/fs/file.h>
#include <core/syscall/syscall.h>

int sys_pipe(int fd[2])
{
    struct inode *pipe_inode = NULL;
    int fd_pipe_write = 0;
    int fd_pipe_read = 0;
    hret_t ret;

    ret = found_free_process_fd(current, &fd_pipe_read);
    if(ret != RET_OK)
        return ret;

    current->open_files[fd_pipe_read] = kmalloc(sizeof(struct file));

    if(current->open_files[fd_pipe_read] == NULL)
        goto free_mem;

    ret = found_free_process_fd(current, &fd_pipe_write);
    if(ret != RET_OK)
        return ret;

    current->open_files[fd_pipe_write] = kmalloc(sizeof(struct file));

    if(current->open_files[fd_pipe_write] == NULL)
        goto free_mem;

    pipe_inode = get_pipe_inode();

    if(!pipe_inode)
        return -ERR_NO_MEM;

    make_pipe(OPEN_READ, current->open_files[fd_pipe_read], pipe_inode);
    make_pipe(OPEN_WRITE, current->open_files[fd_pipe_write], pipe_inode);

    fd[0] = fd_pipe_read;
    fd[1] = fd_pipe_write;

    debugk("[PIPE]: Getting fd %u (read) and fd %u (write)", fd_pipe_read, fd_pipe_write);

    return RET_OK;

free_mem:
    if(current->open_files[fd_pipe_read])
        kfree(current->open_files[fd_pipe_read]);

    if(current->open_files[fd_pipe_write])
         kfree(current->open_files[fd_pipe_write]);

    if(pipe_inode)
        kfree(pipe_inode);
        
    return -ERR_NO_MEM;

}
