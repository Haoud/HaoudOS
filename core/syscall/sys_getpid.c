#include <core/syscall/syscall.h>

int sys_getpid(void)
{
    return current->pid;
}