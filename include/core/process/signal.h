#pragma once
#include <types.h>

struct process;                 // Evite un avertissement ou une sorte inclusion infinie

#define MAX_SIGNALS     32

#define SIGINVAL        0       // Signal invalide
#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGUNUSED	 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU	    22

#define SIG_DFL         0       // Executer l'action par défaut du signal
#define SIG_IGN         1       // Ignorer le signal

#define set_signal(var, signal)   (var) |= ( 1 << (signal - 1))
#define clear_signal(var, signal) (var) &= ~( 1 << (signal - 1))
#define has_signal(var, signal)   (var & (1 << (signal - 1)))
#define valid_signal(signal)      ((signal) > 1 && (signal) < 32) 

hret_t init_signals(struct process *p);         // Initialise les signaux pour un processus
uint32_t dequeue_signal(uint32_t var);          // Renvoit le premier signal trouvé, 0 si aucun signal n'est disponible
hret_t handle_signal(uint32_t signal);          // Traite un signal au nom du processus courant
