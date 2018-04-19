#include <core/fs/file.h>
#include <core/dev/char/tty.h>

int sys_isatty(int fd)
{
    struct file *open_file;
    hret_t ret;
    
    ret = get_open_file(current, fd, &open_file);
    if(open_file == NULL || ret != RET_OK)
        return 0;

    if(open_file->inode->type != INODE_CHAR_DEVICE)
        return 0;
    
    if(GET_MAJOR(open_file->inode->dev) != TTY_MAJOR)
        return 0;

    return 1;
}