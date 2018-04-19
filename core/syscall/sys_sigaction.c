#include <core/syscall/syscall.h>

int sys_sigaction(int _unused signum, uint32_t _unused *action, uint32_t _unused *old_action)
{
    return -ERR_NOT_IMPL;
}
