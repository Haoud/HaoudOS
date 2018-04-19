#pragma once
#include <types.h>
#include <i386/cpu.h>
#include <i386/fpu.h>
#include <core/mm/mm_context.h>
#include <core/process/signal.h>

struct inode;
struct file;

#define USER_CODE_VBASE				0x80000000
#define USER_STACK_VBASE			0xE0000000

#define TASK_UNRUNNABLE					0
#define TASK_CREATED					1
#define TASK_READY						2
#define TASK_RUNNING					3
#define TASK_BLOCKED_INTERRUPTIBLE		4
#define TASK_BLOCKED_UNINTERRUPTIBLE	5
#define TASK_ZOMBIE						6

#define MAX_PROCESS			10
#define MAX_OPENED_FILE		32

struct process
{
	uint32_t pid;
	uint32_t state;
	uint32_t priority;
	uint32_t exit_code;
	uint32_t current_priority;

	char *cwd_path;									// Permet d'implémenter . et ..
	struct file *open_files[MAX_OPENED_FILE];

	uint32_t signal;
	uint32_t sigfn[MAX_SIGNALS];

	struct process *parent;							// Processus parent
	struct process *sibling;						// Liste des processus "frères" de ce processus
	struct process *children;						// Liste des enfants de ce processus

	struct process *sibling_prev, *sibling_next;
	struct process *children_prev, *children_next;

	uint32_t user_tick;
	uint32_t kernel_tick;

	bool_t used_fpu : 1;							// Est ce que le processus a utilisé le FPU au moins un fois ?
	bool_t fpu_state_loaded : 1;					// Est ce que l'état du FPU correspond à celui du processus 
	
	void *code_start;
	void *code_end;									// Note: le "code" comprend aussi les données inclues dans le binaire
	void *brk_start;								// Initialisé lors de la création du processus
	void *brk_end;									// Fin du heap (change dynamiquement) théorique
	void *stack_start;								// Début de la pile (0xE0000000)
	void *stack_end;								// Fin de la pile
	struct kernel_stack kstack;
	struct fpu_regs fpu_contexte;
	struct cpu_state *cpu_contexte;
	struct mm_context *mm_contexte;
};

extern struct process process_list[];
extern struct process *current;

void execute_idle(void);
hret_t free_cwd(struct process *p);
struct process *FindFreeProcessSlot(void);
struct process *PidToProcess(uint32_t pid);
hret_t init_cwd(struct process *p, const char *file);
hret_t copy_cwd(struct process *model, struct process *to);

void remplace_process(char *file);
uint32_t create_and_copy_process(struct process *modele);
void create_process(void *code_location, size_t code_size);
void create_kthread(void *code_location, size_t code_size);
hret_t init_process_tree(struct process *p, struct process *parent);

