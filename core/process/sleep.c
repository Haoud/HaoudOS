#include <lib/list.h>
#include <core/time/timer.h>
#include <core/process/wait.h>
#include <core/process/sleep.h>
#include <core/process/process.h>
#include <core/process/schedule.h>

void sleep_on_timeout(struct wait_queue *q, unsigned int millis)
{
	struct wait_queue wait;

	wait.process = current;
	add_wait_queue(q, &wait);
	schedule_timeout( millis);
	remove_wait_queue(q, &wait);
}

void sleep_on_interruptible(struct wait_queue *q)
{
	struct wait_queue wait;

	current->state = TASK_BLOCKED_INTERRUPTIBLE;

	wait.process = current;
	add_wait_queue(q, &wait);
	schedule();
	remove_wait_queue(q, &wait);
}

void process_timeout(uint32_t data)
{
	struct process *to_wakeup = (struct process *)data;

	if(to_wakeup == current)
		to_wakeup->state = TASK_RUNNING;
	else
		to_wakeup->state = TASK_READY;
}

void schedule_timeout(unsigned int millis)
{
	struct timer tim;
	init_timer(&tim);						// Initialise le timer
	set_timer(&tim, millis);				// Spécifie le temps d'expiration du timer

	tim.data = (uint32_t)current;			// Ainsi que la données à passer lorsque la timer expire
	tim.expire_function = process_timeout;	// Indique la fonction à appeler lorsque le timer expire
	current->state = TASK_BLOCKED_UNINTERRUPTIBLE;

	add_timer(&tim);
	schedule();
	del_timer(&tim);						// Par sécurité, le timer est normalement automatiquement détruit
}

void sleep_on(struct wait_queue *q)
{
	struct wait_queue wait;

	current->state = TASK_BLOCKED_UNINTERRUPTIBLE;

	wait.process = current;
	add_wait_queue(q, &wait);
	schedule();
	remove_wait_queue(q, &wait);
}

void wake_up(struct wait_queue *q)
{
	struct wait_queue *current_wait;
	int nb_element;

	list_foreach(q, current_wait, nb_element)
	{
		if (current_wait->process)
			current_wait->process->state = TASK_READY;
	}
}