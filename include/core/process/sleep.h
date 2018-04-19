#pragma once
#include <types.h>
#include <core/process/wait.h>

void sleep_on_timeout(struct wait_queue *q, unsigned int millis);
void sleep_on_interruptible(struct wait_queue *q);
void schedule_timeout(unsigned int millis);
void sleep_on(struct wait_queue *q);
void wake_up(struct wait_queue *q);
