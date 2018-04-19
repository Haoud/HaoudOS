#include <lib/stdio.h>
#include <core/syscall/syscall.h>

#define F_DUPFD		0
#define F_GETFD		1
#define F_SETFD	    2

int sys_fcntl(int fd, int request_id, int request_arg)
{
    struct file *fd_fcntl;
    hret_t ret;

	ret = get_open_file(current, fd, &fd_fcntl);
	if (ret != RET_OK)
		return ret;

    switch(request_id)
    {
        case F_GETFD:
            return fd_fcntl->open_flags;
            break;

        case F_SETFD:
            fd_fcntl->open_flags = request_arg;
            return RET_OK;
            break;

        default:
            debugk("[FCNTL]: Unknow request %u (0x%x)\n", request_id, request_id);
            return -ERR_NOT_IMPL;
            break;
    }

    return -ERR_UNKNOW;
}
