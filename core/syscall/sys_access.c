#include <lib/stdio.h>
#include <core/mm/heap.h>
#include <core/fs/inode.h>
#include <core/syscall/syscall.h>

#define	F_OK		0		    // Teste l'existance du fichier
#define	X_OK		0x01	    // Teste l'accès en éxécution
#define	W_OK		0x02	    // Teste l'accès en écriture
#define	R_OK		0x04	    // Teste l'accès en lecture

int sys_access(const char *path, int access)
{
    struct inode *res_inode = NULL;
    char *remaning_path = NULL;
    hret_t ret;
    
	debugk("[ACCESS]:%s with access %02x\n", path, access);

    switch(access)
    {
        case F_OK:
            ret = lookup_inode(path, &remaning_path, &res_inode);

            if(ret != RET_OK)
                return ret;

            if(remaning_path != NULL)
            {
                kfree(remaning_path);
                return -ERR_NOT_FOUND;
            }

            return 0;

        default:
            if((access & R_OK) || (access & W_OK) || (access & X_OK))
                return 0;

            debugk("[ACCESS]: Unknown value 0x%02x(%u)", access, access);
            return -ERR_BAD_ARG;
    }
}
