#include <types.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <core/process/signal.h>
#include <core/process/process.h>
#include <core/syscall/syscall.h>

hret_t init_signals(struct process *p)
{
    p->signal = 0;
    for(int i = 0; i < MAX_SIGNALS; i++)
        p->sigfn[i] = SIG_DFL; 

    return RET_OK;
}

uint32_t dequeue_signal(uint32_t var)
{
    if(var)
    {
        for(int i = 1; i < MAX_SIGNALS; i++)
        {
            if(has_signal(var, i))
                return i;
        }        
    }

    return 0;
}

hret_t handle_signal(uint32_t signal)
{
    if(!valid_signal(signal))
        return -ERR_BAD_ARG;
    
    if(current->sigfn[signal] == SIG_IGN)
        goto clear_signal;
    else if(current->sigfn[signal] == SIG_DFL)
    {
        // Execute les action par défaut des signaux
        switch(signal)
        {
            case SIGSEGV:
                printk("Segmentation fault\n");
                sys_exit(-1);
                break;

            case SIGFPE:
                printk("Floating point error\n");
                sys_exit(-1);
                break;

            case SIGINT: case SIGHUP: case SIGQUIT:
                sys_exit(1);
                break;

            case SIGCHLD:
                goto out;
                break;

            default:
                goto clear_signal;
                break;
        }
    }
    else
    {
        printk("Custom handler for signal not supported\n");
        // Sauvegarder les registres dans la pile utilisateur puis modifier les registres

        current->sigfn[signal] = SIG_DFL;               // Remet le signal par défaut
        if(signal != SIGCHLD)
            clear_signal(current->signal, signal);

        goto out;       
    }

clear_signal:
    clear_signal(current->signal, signal);
out:
    return RET_OK;
}

void do_signal(struct cpu_state _unused *regs)
{
    // Traite un signal du processus courant
    handle_signal(dequeue_signal(current->signal));
}