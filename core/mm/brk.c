#include <core/mm/brk.h>
#include <core/mm/phys.h>
#include <core/process/process.h>

void init_brk(struct process *p)
{
	p->brk_start =  0;
	p->brk_end = 	0;
}

/*
 * 
 */
uint32_t extend_brk(void *addr)
{
	if (addr == NULL)
		return (uint32_t)current->brk_end;

	if (addr == current->brk_end)
		return (uint32_t)current->brk_end;

	if (addr < current->brk_start)
		return 0;

	if(current->brk_end == 0)
	{
		current->brk_start = addr;					
		current->brk_end = addr;
		return (uint32_t)current->brk_end;			
	}
	
	if (addr > current->brk_end)
	{
		current->brk_end = addr;
	}
	else
	{
		// On ne désalloue pas les pages déjà allouées
		current->brk_end = addr;
	}

	return (uint32_t)current->brk_end;			
}
