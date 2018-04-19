#include <core/mm/heap.h>
#include <core/fs/inode.h>
#include <lib/stdio.h>

int sys_stat(const char *path, struct stat *stat_struct)
{
    struct inode *result_inode = NULL;
    char *remaning_path = NULL;
    hret_t ret;

	debugk("[STAT]: %s\n", path);

    ret = lookup_inode(path, &remaning_path, &result_inode);

    if(ret != RET_OK)
        return ret;

    if(remaning_path != NULL)
    {
        kfree(remaning_path);
        return -ERR_NOT_FOUND;
    }
    
    ret = result_inode->inode_op->stat(result_inode, stat_struct);
    return ret;
}          
