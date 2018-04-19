#include <types.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <core/fs/file.h>
#include <core/mm/area.h>
#include <core/process/process.h>
#include <core/process/schedule.h>

void sys_exit(int exit_code)
{
	if (current->pid == 0)
		panic("La tâche système tente de s'arrêter");

	debugk("[SYSCALL]: Process pid %u exiting with code 0x%08x\n", current->pid, exit_code);

	// On signalise le processus comme étant un zombie
	current->state = TASK_ZOMBIE;
	current->exit_code = exit_code;
	
	// Indique au parent qu'un de ses fils est mort
	if(current->parent != NULL)
		set_signal(current->parent->signal, SIGCHLD);
	else
		printk("Process pid %u exiting without valid parent !", current->pid);

	// Tous les fils du processus mort sont adopté par le processus init (pid 1)
	if(!list_empty(current->children))
	{
		struct process *pnext;
		struct process *curp;
		int nb_process;

		list_foreach_safe_named(current->children, curp, pnext, nb_process, children_prev, children_next)
		{
			curp->parent = &process_list[2];
			list_delete_named(current->children, curp, children_prev, children_next);
			list_add_after_named(process_list[2].children, curp, children_prev, children_next);
		}
	}

	mm_contexte_SwitchTo(process_list[0].mm_contexte);
	mm_context_unref(current->mm_contexte);
	free_cwd(current);

	// On libère les descripteurs de fichier
	for (int i = 0; i < MAX_OPENED_FILE; i++)
	{
		if (current->open_files[i] != NULL)
			unref_open_file(current->open_files[i]);
	}

	// La pile noyau du processus sera libéré lors d'un appel à wait
	schedule();
}