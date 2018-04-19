#include <core/syscall/syscall.h>

int sys_fstat(int fd, struct stat *stat_struct)
{
    struct file *opfile = NULL;
    hret_t ret;
    
    ret = get_open_file(current, fd, &opfile);
    if(ret != RET_OK)
        return ret;

    if(opfile->inode->inode_op == NULL)
        return -ERR_NOT_IMPL;
    
    if(opfile->inode->inode_op->stat == NULL)
        return -ERR_NOT_IMPL;
    
    ret = opfile->inode->inode_op->stat(opfile->inode, stat_struct);
    return ret;
}
