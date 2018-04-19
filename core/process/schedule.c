#include <i386/idt.h>
#include <i386/fpu.h>
#include <i386/tss.h>
#include <i386/i8254.h>
#include <i386/i8259.h>
#include <driver/bochs/bochs.h>
#include <core/process/process.h>
#include <core/process/schedule.h>

extern void asm_SchedulerCore(void);

/*
 * Cette fonction tr�s simple initialise le scheduler et son vecteur d'interruption
 */
void SchedulerSetup(void)
{
	MakeIdtDescriptor(SCHEDULER_VECTOR, (uint32_t)&asm_SchedulerCore, 0x08, IDT_HANDLER_PRESENT | IDT_INT_GATE32);
	return;
}

/*
 * Cette fonction est le coeur du scheduler et premet de suspendre un processus et de le
 * remplacer par un autre
 */
uint32_t SchedulerCore(struct cpu_state *current_state)
{
	struct process *p;
	// On r�veille les processus en attente interruptible qui ont des signaux en attente
	for (int i = 0; i < MAX_PROCESS; i++)
	{
		if (process_list[i].signal && process_list[i].state == TASK_BLOCKED_INTERRUPTIBLE)
			process_list[i].state = TASK_READY;
	}

	p = FindEligibleProcess();

	// On actualise l'emplacement de la liste des registres sauvegard�s
	current->cpu_contexte = current_state;	
	
	// Si le processus est le m�me qu'actuellement alors pas besoin de changer de processus
	if (p == current)
		return (uint32_t)current->cpu_contexte;

	save_fpu_state();

	// Sauvegarde du TSS
	current->kstack.ss0 = tss_kernel.ss0;
	current->kstack.esp0 = tss_kernel.esp0;

	if (current->state == TASK_RUNNING)			// Si le processus n'est pas en attente
		current->state = TASK_READY;			// Le processus qui �tait en train de s'�x�cuter est d�clar� comme pr�t

	current = p;
	current->state = TASK_RUNNING;			// Le processus �lu est d�clar� comme en cours d'�x�cution

	// Restoration du TSS
	tss_kernel.esp0 = current->kstack.esp0;
	tss_kernel.ss0 = current->kstack.ss0;
	
	// Changement de contexte m�moire
	mm_contexte_SwitchTo(current->mm_contexte);

	// Et on indique quelle pile noyau charger
	return (uint32_t)current->cpu_contexte;
}

/*
 * Cette fonction se charge de trouver un processus �ligible, c'est � dire lui
 * restant du temps processeur de disponible
 */
struct process *FindEligibleProcess(void)
{
	if (current->state == TASK_RUNNING)
	{
		if (current->current_priority != 0)
			return current;
	}

	// Ici on doit trouver un nouveau processus
	for (int i = 0; i < MAX_PROCESS; i++)
	{
		// Si on trouve  un processus pr�t avec du temps processeur restant, c'est OK pour nous ;-)
		if (process_list[i].current_priority && process_list[i].state == TASK_READY)
			return &process_list[i];
	}

	// Si on arrive ici, c'est que tous les processus �ligible ont consomm� leur temps processeur
	// On remet donc les compteurs � leurs valeurs initiales
	for (int i = 0; i < MAX_PROCESS; i++)
	{
		process_list[i].current_priority = process_list[i].priority;
	}

	// Ici on essaye de nouveau de trouver un processus libre
	for (int i = 0; i < MAX_PROCESS; i++)
	{
		// Si on trouve  un processus pr�t avec du temps processeur restant, c'est OK pour nous ;-)
		if (process_list[i].current_priority && process_list[i].state == TASK_READY)
			return &process_list[i];
	}

	//Si on arrive ici, seul le processus actuel est disponible donc on le retourne
	return current;
}