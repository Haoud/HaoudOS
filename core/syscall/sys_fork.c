#include <lib/list.h>
#include <lib/stdio.h>
#include <core/syscall/syscall.h>
#include <core/process/process.h>

int sys_fork(void)
{
	struct process *new_process;		// Processus nouvelemnt crée
	uint32_t new_ppid;					// PID du processus nouvellement crée

	debugk("[SYSCALL]: Process pid %u forking...\n", current->pid);

	new_ppid = create_and_copy_process(current);
	new_process = PidToProcess(new_ppid);

	debugk("[SYSCALL]: Process pid %u has new child (pid %u)..\n", current->pid, new_ppid);	

	// On modifie EAX du fils, EAX contient maintenant 0
	new_process->cpu_contexte->eax = 0;
	return new_ppid;
}