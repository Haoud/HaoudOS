#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/mm/area.h>
#include <core/process/sleep.h>
#include <core/syscall/syscall.h>
#include <core/process/schedule.h>

int sys_waitpid(int pid, int *status)
{
    struct process *children;
    int nb_process;

    if(list_empty(current->children))
        return -ERR_NO_CHILD;

    if(pid > 1)
    {
        debugk("[WAIT] Process pid %u wait child pid %u\n", current->pid, pid);

        list_foreach_named(current->children, children, nb_process, children_prev, children_next)
        {
            if(children->pid == (unsigned int)pid)
                goto child_found;               
        }

        return -ERR_BAD_ARG;

child_found:
        if(children->state == TASK_ZOMBIE)
        {
            // Indique le code de retour du processus
            *status = children->exit_code;

            // Libère la pile noyau du processus
            PagingUnmap(children->kstack.esp0 - PAGE_SIZE);
            FreeVmArea(children->kstack.esp0 - PAGE_SIZE);

            // On indique la structure du processus est "libre"
            children->state = TASK_UNRUNNABLE;

            list_delete_named(current->children, children, children_prev, children_next);
            clear_signal(current->signal, SIGCHLD);

            return children->pid;
        }

        // On attend le signal SIGCHLD
        while(!has_signal(current->signal, SIGCHLD) && children->state != TASK_ZOMBIE)
        {
            current->state = TASK_BLOCKED_INTERRUPTIBLE;
            schedule();
        }

        // Indique le code de retour du processus
        *status = children->exit_code;

        // Libère la pile noyau du processus
        PagingUnmap(children->kstack.esp0 - PAGE_SIZE);
        FreeVmArea(children->kstack.esp0 - PAGE_SIZE);

        // On indique la structure du processus est "libre"
        children->state = TASK_UNRUNNABLE;

        list_delete_named(current->children, children, children_prev, children_next);
        clear_signal(current->signal, SIGCHLD);
        return children->pid;
    }
    else if(pid == 0)
    {
        printk("sys_wait() with pid 0 not implemented");
        return -ERR_NOT_IMPL;
    }
    else if(pid == -1)
    {        

        debugk("[WAIT] Process pid %u wait any child\n", current->pid);

        // On attend la mort de n'importe quel processus fils
        // On vérifie s'il y a déjà un fils mort
        list_foreach_named(current->children, children, nb_process, children_prev, children_next)
        {
            if(children->state == TASK_ZOMBIE)
            {
                // Indique le code de retour du processus
                *status = children->exit_code;

                // Libère la pile noyau du processus
                PagingUnmap(children->kstack.esp0 - PAGE_SIZE);
                FreeVmArea(children->kstack.esp0 - PAGE_SIZE);

                // On indique la structure du processus est "libre"
                children->state = TASK_UNRUNNABLE;

                list_delete_named(current->children, children, children_prev, children_next);
                clear_signal(current->signal, SIGCHLD);
                debugk("[WAIT]: Child found without sleeping (pid %u)", children->pid);
                return children->pid;
            }
        }

        // On attend le signal SIGCHLD
        while(!has_signal(current->signal, SIGCHLD))
        {
            current->state = TASK_BLOCKED_INTERRUPTIBLE;
            schedule();
        }

        
       list_foreach_named(current->children, children, nb_process, children_prev, children_next)
        {
            if(children->state == TASK_ZOMBIE)
            {
                // Indique le code de retour du processus
                *status = children->exit_code;

                // Libère la pile noyau du processus
                PagingUnmap(children->kstack.esp0 - PAGE_SIZE);
                FreeVmArea(children->kstack.esp0 - PAGE_SIZE);

                // On indique la structure du processus est "libre"
                children->state = TASK_UNRUNNABLE;

                list_delete_named(current->children, children, children_prev, children_next);
                clear_signal(current->signal, SIGCHLD);
                debugk("[WAIT]: Child found with sleeping");
                return children->pid;
            }
        }

        return -ERR_NOT_FOUND;
    }

    return -ERR_BAD_ARG;
}