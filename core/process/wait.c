#include <types.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <core/mm/heap.h>
#include <core/process/wait.h>

void init_wait_queue(struct wait_queue **q)
{
	(*q) = kmalloc(sizeof(struct wait_queue));

	(*q)->process = NULL;
	list_singleton(*q, *q);
}

hret_t release_wait_queue(struct wait_queue *q)
{
	if(!list_empty(q))
		return -ERR_BUSY;

	kfree(q);
	return RET_OK;
}

void add_wait_queue(struct wait_queue *q, struct wait_queue *entry)
{
	uint32_t flags;
	lock_int(flags);				// Empêche une interruption de survenir

	if(list_empty(q))
		list_singleton(q, entry);
	else
		list_add_after(q, entry);

	unlock_int(flags);				// Remet les interruptions comme avant
}

void remove_wait_queue(struct wait_queue *q, struct wait_queue *entry)
{
	uint32_t flags;
	lock_int(flags);				// Empêche une interruption de survenir

	if (entry->process == NULL)
		panic("wait_queue: Tentative de libération du pointeur de la liste");

	list_delete(q, entry);

	unlock_int(flags);				// Remet les interruptions comme avant
}