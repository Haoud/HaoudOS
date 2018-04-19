#include <core/mm/brk.h>
#include <core/syscall/syscall.h>

int sys_brk(void *addr)
{
	return extend_brk(addr);
}