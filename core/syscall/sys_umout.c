#include <core/fs/file_system.h>
#include <core/syscall/syscall.h>

int sys_umount(const char drive_src)
{
	return umount(drive_src);
}