#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/process/signal.h>
#include <core/syscall/syscall.h>
#include <core/process/process.h>

int sys_kill(int pid, int signal)
{
    struct process *to_send;
    
    if(pid > 1)
    {
        to_send = PidToProcess(pid);
        if(to_send == NULL)
            return -ERR_BAD_ARG;
        
        if(!valid_signal(signal))
            return -ERR_BAD_ARG;

        set_signal(to_send->signal, signal);

        return RET_OK;
    }
    else if(pid == 0)
    {
        printk("sys_kill() with pid 0 not implemented");
        return -ERR_NOT_IMPL;
    }
    else if(pid == -1)
    {
        struct process *curp;
        int nb_process;
        
        // Si le processus a des frères
        if(!list_empty(current->sibling))
        {
            // Envoie le signal a tous les processus du même père
            list_foreach_named(current->sibling, curp, nb_process, sibling_prev, sibling_next)
                set_signal(curp->signal, signal);
            
        }

        return RET_OK;
    }

    return -ERR_BAD_ARG;
}