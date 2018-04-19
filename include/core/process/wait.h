#pragma once
#include <types.h>
#include <core/process/process.h>

struct wait_queue
{
	struct process *process;

	struct wait_queue *prev;
	struct wait_queue *next;
};

void init_wait_queue(struct wait_queue **q);
hret_t release_wait_queue(struct wait_queue *q);
void add_wait_queue(struct wait_queue *q, struct wait_queue *entry);
void remove_wait_queue(struct wait_queue *q, struct wait_queue *entry);