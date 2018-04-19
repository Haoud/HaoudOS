#include <types.h>
#include <lib/stdio.h>

int sys_debug(int gravity, const char *msg)
{
    debugk("[USER DEBUG %i]: %s", gravity, msg);
    return RET_OK;
}
