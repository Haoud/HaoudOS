#pragma once

#define SCHEDULER_VECTOR		0x81							// Numéro d'interruption associé au scheduler

#define schedule()			\
	asm("int %0" :: "i"(SCHEDULER_VECTOR) : "memory")

void SchedulerCall(void);										// Change de contexte CPU et mémoire
void SchedulerSetup(void);										// Initialise le scheduler
struct process *FindEligibleProcess(void);
uint32_t SchedulerCore(struct cpu_state *current_state);		// Charge de trouver quel processus sera préempté
