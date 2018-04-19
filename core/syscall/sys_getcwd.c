#include <lib/string.h>
#include <core/process/process.h>
#include <core/syscall/syscall.h>

int sys_getcwd(char *buffer, size_t buffer_size)
{
	size_t cwd_size = strlen(current->cwd_path) + 1;

	if (cwd_size > buffer_size)
		return -ERR_RANGE;

	strcpy(buffer, current->cwd_path);	
	return RET_OK;
}