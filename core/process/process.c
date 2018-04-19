#include <assert.h>
#include <i386/tss.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <lib/stdlib.h>
#include <core/mm/brk.h>
#include <core/fs/file.h>
#include <core/mm/heap.h>
#include <driver/vga/text.h>
#include <core/process/elf.h>
#include <core/process/process.h>

static uint32_t next_pid = 0;

struct process process_list[MAX_PROCESS];
struct process *current = &process_list[0];			// Par défaut

/*
 * Cette fonction permet d'éxécuter la tâche idle, c'est à dire la première tâche
 * créer par HaoudOS et qui ne fait rien (sauf gaspiller du temps processeur)
 */
void execute_idle(void)
{
	current = &process_list[0];					// Le processus 0 est toujours la tâche idle

	current->state = TASK_RUNNING;
	vaddr_t esp0 = (vaddr_t) current->cpu_contexte;

	// Mise à jour du TSS
	tss_kernel.esp0 = current->kstack.esp0;
	tss_kernel.ss0 = current->kstack.ss0;

	mm_contexte_SwitchTo(current->mm_contexte);
	
	// Changement de contexte matériel
	asm("								\n\
		mov esp, %0						\n\
		pop ss							\n\
		pop gs							\n\
		pop fs							\n\
		pop es							\n\
		pop ds							\n\
		popad							\n\
		iretd 		" :: "m"(esp0) : "memory", "eax", "ebx", "ecx", "edx", "edi", "esi");
}

/*
 * Cette fonction permet de créer la copie conforme d'un processus existant en duplicant
 *	- Le contenu de sa mémoire
 *	- Les registres sauvegardés
 *	- Les descripteurs de fichier ouverts
 *
 * La pile noyau n'est pas copié pour des raisons de sécurité: chaque processu prossède SA
 * PROPRE PILE NOYAU
 * Retourne le PID du processus nouvellement crée
 */
uint32_t create_and_copy_process(struct process *modele)
{
	vaddr_t kstack = 0;
	struct process *p = NULL;
	struct mm_context *old_mmc = NULL;
	struct cpu_user_state *cpu = NULL;

	p = FindFreeProcessSlot();

	if (!p)
		goto undo_creation;

	p->mm_contexte = mm_context_duplicate(modele->mm_contexte);
	if (!p->mm_contexte)
		goto undo_creation;

	old_mmc = current_mm_context;
	mm_contexte_SwitchTo(p->mm_contexte);

	// Code de la création du processus
	p->pid = next_pid++;
	p->state = TASK_READY;
	p->priority = 10;
	p->current_priority = 10;

	// Création et mapping de la pile noyau
	kstack = get_vm_area(1, 0);

	if (!kstack)
		goto undo_creation;

	// On met à jour les champs du processus concernant la pile noyau
	p->kstack.esp0 = kstack + PAGE_SIZE;
	p->kstack.ss0 = 0x10;

	// Mapping de la pile noyau
	PagingAutoMap(kstack, 1, VM_MAP_WRITE);

	// On enregistre le contexte mémoire utilisateur sur la pile noyau
	cpu = (struct cpu_user_state *) ((p->kstack.esp0) - sizeof(struct cpu_user_state));
	
	cpu->esp3 = ((struct cpu_user_state*)modele->cpu_contexte)->esp3;
	cpu->ss3 = ((struct cpu_user_state*)modele->cpu_contexte)->ss3;

	// On copie TOUT les registres
	cpu->regs.eip = modele->cpu_contexte->eip;
	cpu->regs.eax = modele->cpu_contexte->eax;
	cpu->regs.ebx = modele->cpu_contexte->ebx;
	cpu->regs.ecx = modele->cpu_contexte->ecx;
	cpu->regs.edx = modele->cpu_contexte->edx;

	cpu->regs.cs = modele->cpu_contexte->cs;
	cpu->regs.ds = modele->cpu_contexte->ds;
	cpu->regs.es = modele->cpu_contexte->es;
	cpu->regs.fs = modele->cpu_contexte->fs;
	cpu->regs.gs = modele->cpu_contexte->gs;
	cpu->regs.ss = modele->cpu_contexte->ss;

	cpu->regs.edi = modele->cpu_contexte->edi;
	cpu->regs.esi = modele->cpu_contexte->esi;
	cpu->regs.esp = modele->cpu_contexte->esp;
	cpu->regs.ebp = modele->cpu_contexte->ebp;
	cpu->regs.eflags = modele->cpu_contexte->eflags;	

	p->code_start = modele->code_start;
	p->code_end = modele->code_end;
	p->brk_start = modele->brk_start;
	p->brk_end = modele->brk_end;
	p->stack_start = modele->stack_start;
	p->stack_end = modele->stack_end;

	p->fpu_state_loaded = FALSE;
	p->used_fpu = FALSE;
	
	p->cpu_contexte = (struct cpu_state *)cpu;			// Registre sauvegardé de ce processus
	duplicate_fd_struct(modele, p);						// Copie les descripteur de fichier ouvert
	copy_cwd(modele, p);
	init_process_tree(p, modele);

	init_signals(p);
	p->signal = current->signal;

	mm_contexte_SwitchTo(old_mmc);						// Repasse au contexte mémoire précédent

	return p->pid;

undo_creation:
	if (p)
	{
		p->state = TASK_UNRUNNABLE;

		// Libération du contexte mémoire (s'il il existe)
		if (p->mm_contexte)
		{
			mm_context_unref(p->mm_contexte);
			if (old_mmc)
				mm_contexte_SwitchTo(old_mmc);
		}

		//Libération de la pile noyau (s'il elle existe)
		if (kstack)
			FreeVmArea(kstack);
	}
	return 0xFFFFFFFF;
}

/*
 * Cette fonction permet de créer un processus utilisateur à partir d'une fonction
 * situé dans le noyau: cette fonction ne peut pas appeler les autres fonctions du
 * noyau
 *
 * code_location est l'adresse de départ du code
 * code_size est la taille du code (doit être au moins égal à la taille de la fonction)
 */
void create_process(void *code_location, size_t code_size)
{
	vaddr_t kstack = 0;
	uint32_t code_pg_size;
	struct process *p = NULL;
	struct mm_context *old_mmc = NULL;
	struct cpu_user_state *cpu = NULL;

	code_pg_size = (code_size / PAGE_SIZE) + 1;

	p = FindFreeProcessSlot();
	if (!p)
		goto undo_creation;

	p->mm_contexte = create_mm_context();
	if (!p->mm_contexte)
		goto undo_creation;

	old_mmc = current_mm_context;
	mm_contexte_SwitchTo(p->mm_contexte);

	// Code de la création du processus
	p->pid = next_pid++;
	p->state = TASK_READY;
	p->priority = 10;
	p->current_priority = 10;

	// Création et mapping de la pile noyau
	kstack = get_vm_area(1, 0);
	if (!kstack)
		goto undo_creation;

	// On met à jour les champs du processus concernant la pile noyau
	p->kstack.esp0 = kstack + PAGE_SIZE;
	p->kstack.ss0 = 0x10;

	// Mapping de la pile noyau
	PagingAutoMap(kstack, 1, VM_MAP_WRITE);

	// On enregistre le contexte mémoire utilisateur sur la pile noyau
	cpu = (struct cpu_user_state *) ((p->kstack.esp0) - sizeof(struct cpu_user_state));

	// On crée la pile utilisateur
	cpu->esp3 = USER_STACK_VBASE - 16;
	cpu->ss3 = 0x2B;

	// On mappe la pile utilisateur (une page pour l'instant)
	PagingAutoMap(USER_STACK_VBASE - PAGE_SIZE, 1, VM_MAP_USER | VM_MAP_WRITE);

	// On initialise EIP ...
	cpu->regs.eip = USER_CODE_VBASE;
	PagingAutoMap(USER_CODE_VBASE, code_pg_size, VM_MAP_USER | VM_MAP_WRITE);	// On prépare la zone qui contiendra le code ...
	memcpy((void *)USER_CODE_VBASE, code_location, code_size);					// Puis on recopie le code à la bonne adresse

	// On initialise les segments
	cpu->regs.cs = 0x23;
	cpu->regs.ds = 0x2B;
	cpu->regs.es = 0x2B;
	cpu->regs.fs = 0x2B;
	cpu->regs.gs = 0x2B;
	cpu->regs.ss = 0x10;				// Pile noyau car nous somme (théoriquement) dans un gestionnaire d'interruption, qui met à jour SS avec la valeur contenu dans le TSS.ss0

	// On initialise les registres généraux à zéro
	cpu->regs.eax = 0;
	cpu->regs.ebx = 0;
	cpu->regs.ecx = 0;
	cpu->regs.edx = 0;

	cpu->regs.esp = 0;					// Ce champs importe peu en mode utilisateur car la pile utilisateur est sauvegardée par IRETD
	cpu->regs.ebp = 0;
	cpu->regs.esi = 0;
	cpu->regs.edi = 0;

	cpu->regs.eflags = EFLAGS_IF;		// Autorise les interruptions pour le processus
	p->cpu_contexte = (struct cpu_state *)cpu;

	p->code_start = code_location;
	p->code_end = (char *)p->code_start + code_size;

	p->stack_start = (void *)USER_STACK_VBASE;
	p->stack_end = (void *)(USER_STACK_VBASE - PAGE_SIZE);

	p->fpu_state_loaded = FALSE;
	p->used_fpu = FALSE;

	mm_contexte_SwitchTo(old_mmc);		// Repasse au contexte mémoire précédent
	init_fd_struct(p);
	init_cwd(p, "C:\\");
	init_brk(p);
	init_signals(p);
	init_process_tree(p, current);

	return;

undo_creation:
	if (p)
	{
		p->state = TASK_UNRUNNABLE;

		// Libération du contexte mémoire (s'il il existe)
		if (p->mm_contexte)
		{
			mm_context_unref(p->mm_contexte);
			if (old_mmc)
				mm_contexte_SwitchTo(old_mmc);
		}

		//Libération de la pile noyau (s'il elle existe)
		if (kstack)
			FreeVmArea(kstack);
	}
	return;
}

/*
 * Cette fonction est similaire à create_process sauf que le processus sera un thread noyau:
 * il possèdera tous les privilèges et peut appeler d'autres fonctions du noyau.
 * Doit être utilisé avec prudence
 */
void create_kthread(void *code_location, size_t code_size)
{
	vaddr_t kstack = 0;
	struct process *p = NULL;
	struct cpu_state *cpu = NULL;

	p = FindFreeProcessSlot();
	if (!p)
		goto undo_creation;

	p->mm_contexte = current_mm_context;
	mm_context_ref(p->mm_contexte);

	// Code de la création du processus
	p->pid = next_pid++;
	p->state = TASK_READY;
	p->priority = 10;
	p->current_priority = 10;

	// Création et mapping de la pile noyau
	kstack = get_vm_area(1, 0);
	if (!kstack)
		goto undo_creation;

	// On met à jour les champs du processus concernant la pile noyau
	p->kstack.esp0 = kstack + PAGE_SIZE;
	p->kstack.ss0 = 0x10;

	// Mapping de la pile noyau
	PagingAutoMap(kstack, 1, VM_MAP_WRITE);

	// On enregistre le contexte mémoire utilisateur sur la pile noyau
	cpu = (struct cpu_state *) ((p->kstack.esp0) - sizeof(struct cpu_state));

	// On initialise EIP ...
	cpu->eip = (uint32_t)code_location;

	// On initialise les segments
	cpu->cs = 0x08;
	cpu->ds = 0x10;
	cpu->es = 0x10;
	cpu->fs = 0x10;
	cpu->gs = 0x10;
	cpu->ss = 0x10;				// Pile noyau car nous somme (théoriquement) dans un gestionnaire d'interruption, qui met à jour SS avec la valeur contenu dans le TSS

	// On initialise les registres généraux à zéro
	cpu->eax = 0;
	cpu->ebx = 0;
	cpu->ecx = 0;
	cpu->edx = 0;

	cpu->esp = 0;				// Ce champs importe peu car popad ne restore pas le pointeur de pile
	cpu->ebp = 0;
	cpu->esi = 0;
	cpu->edi = 0;

	p->code_start = code_location;
	p->code_end = (char *)p->code_start + code_size;

	p->stack_start = (void *)p->kstack.esp0;
	p->stack_end = (void *)(p->kstack.esp0 - PAGE_SIZE);

	p->fpu_state_loaded = FALSE;
	p->used_fpu = FALSE;

	cpu->eflags = EFLAGS_IF;	// Autorise les interruptions pour le processus
	p->cpu_contexte = cpu;
	init_fd_struct(p);
	init_cwd(p, "C:\\");
	init_brk(p);
	init_signals(p);
	init_process_tree(p, current);

	return;

undo_creation:
	if (p)
	{
		p->state = TASK_UNRUNNABLE;

		// Libération du contexte mémoire (s'il il existe)
		if (p->mm_contexte)
		{
			mm_context_unref(p->mm_contexte);
		}

		//Libération de la pile noyau (s'il elle existe)
		if (kstack)
			FreeVmArea(kstack);
	}
	return;
}

/*
 * Cette fonction permet de cherche un emplacement de processus disponible et le retourne
 * s'il en existe au moins un, sinon retourne NULL
 */
struct process *FindFreeProcessSlot(void)
{
	for (int i = 0; i < MAX_PROCESS; i++)
	{
		if (process_list[i].state == TASK_UNRUNNABLE)
			return &process_list[i];
	}

	return NULL;
}

struct process *PidToProcess(uint32_t pid)
{
	for (int i = 0; i < MAX_PROCESS; i++)
	{
		if (process_list[i].pid == pid)
			return &process_list[i];
	}

	return NULL;
}

hret_t init_cwd(struct process *p, const char *file)
{
	p->cwd_path = kmalloc(strlen(file));
	memcpy(p->cwd_path, file, strlen(file));
	int i;

	for (i = strlen(file); p->cwd_path[i] != '\\'; i--)
		nop();

	p->cwd_path[i + 1] = 0;				// On garde le \ de fin

	return RET_OK;
}

hret_t copy_cwd(struct process *model, struct process *to)
{
	int len = strlen(model->cwd_path) + 1;

	to->cwd_path = kmalloc(len);
	memcpy(to->cwd_path, model->cwd_path, len);

	return RET_OK;
}

hret_t free_cwd(struct process *p)
{
	kfree(p->cwd_path);
	return RET_OK;
}

hret_t init_process_tree(struct process *p, struct process *parent)
{
	list_init(p->children);						// Le processus nouvellement crée n'a pas d'enfant
	if(p->pid <= 2)								// On n'itialise pas le parent des processys crée par le noyau au démarrage
	{
		p->parent = NULL;
	}
	else
	{
		p->parent = parent;				// Le parent du processus est le processus actuel

		// On ajoute le processu au enfant du processus parent
		if(list_empty(parent->children))
			list_singleton_named(parent->children, p, children_prev, children_next);
		else
			list_add_after_named(parent->children, p, children_prev, children_next);

		// On initialise la liste des processus frères
		p->sibling = parent->children;
	}

	return RET_OK;
}
