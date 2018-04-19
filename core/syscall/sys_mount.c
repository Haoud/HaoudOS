#include <core/fs/file_system.h>
#include <core/syscall/syscall.h>

int sys_mount(const char *source, const char drive_dst, const char *file_system)
{
	return mount(drive_dst, file_system, source);
}