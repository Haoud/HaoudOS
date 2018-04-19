#include <types.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <lib/string.h>
#include <core/mm/brk.h>
#include <core/fs/vfs.h>
#include <core/mm/heap.h>
#include <core/process/elf.h>
#include <core/mm/mm_context.h>
#include <core/process/process.h>
#include <core/syscall/syscall.h>

/*
 * Retourne le nombre de pointeurs que contient un tableau de pointeur se terminant
 * par un pointeur NULL
 */
int get_array_of_ptr_size(char *tab[])
{
	int i = 0;

	while (tab[i] != NULL)
		i++;

	return i;
}

int get_arg_length(char *tab[], int index)
{
	return strlen(tab[index]) + 1;
}

int sys_exec(const char *path, char *argv[], char *env[])
{
	char *esp = NULL;								// Utilisé pour la copie des arguments & de l'environnement
	uint32_t saved_esp = 0;							// Pour restaurer l'image du processus en cas d'erreur
	char *kernel_path = NULL;						// Chemin d'accès sauvegardé
	char *kernel_argv = NULL;						// Buffer brut des arguments
	char *kernel_envp = NULL;						// Buffer brut de l'environnement
	char *parsed_path = NULL;
	bool_t relative_path = FALSE;

	int kernel_argv_index = 0;
	int kernel_envp_index = 0;

	struct cpu_user_state *cpu = (struct cpu_user_state *)current->cpu_contexte;

	debugk("[EXEC]: Loading %s\n", path);

	hret_t ret_code;
	vaddr_t stack_end;								// Fin de la pile (vers les adresses basses)
	uint32_t argument_size = 0;						// Taille total des arguments en octets
	bool_t has_argument = FALSE;					// Des arguments ont t-ils été passé ?
	uint32_t argument_length = 0;					// Nombre d'arguments passés
	uint32_t environnement_size = 0;				// Taille total en octet de l'environnement
	bool_t has_environnement = FALSE;				// Un environnement a t-il été spécifié
	uint32_t environnement_lenght = 0;				// Nombre de variable d'environnement
	size_t stack_required_length = 4096;			// Taille minimum de la pile (1 page par défaut)

	vaddr_t stack_argv_start = 0;
	vaddr_t stack_envp_start = 0;

	vaddr_t stack_argv_data_start = 0;
	vaddr_t stack_envp_data_start = 0;

	struct mm_context *new_context;

	if(path == NULL)
		return -ERR_BAD_ARG;

	if(path[1] == ':')
		relative_path = FALSE;
	else
		relative_path = TRUE;

	saved_esp = cpu->esp3;

	if (argv != NULL)
	{
		argument_length = get_array_of_ptr_size(argv);

		for (unsigned int i = 0; i < argument_length; i++)
			argument_size += get_arg_length(argv, i);
		
		stack_required_length += argument_size;
		has_argument = TRUE;
	}

	if (env != NULL)
	{
		environnement_lenght = get_array_of_ptr_size(env);

		for (unsigned int i = 0; i < environnement_lenght; i++)
			environnement_size += get_arg_length(env, i);

		stack_required_length += environnement_size;
		has_environnement = TRUE;
	}

	stack_required_length += 64;									// Marge de sécurité
	stack_required_length += argument_length * PTR_SIZE;			// Prend aussi en compte le tableau de pointeur qui sera sur la pile
	stack_required_length += environnement_lenght * PTR_SIZE;		// Idem 

	// Allocation des buffers
	kernel_argv = kmalloc(argument_size);
	kernel_envp = kmalloc(environnement_size);

	kernel_path = kmalloc(strlen(path) + 1);
	strcpy(kernel_path, path);

	// Copie des arguments dans le buffer noyau
	for (unsigned int i = 0; i < argument_length; i++)
	{
		memcpy(&kernel_argv[kernel_argv_index], argv[i], get_arg_length(argv, i));
		kernel_argv_index += get_arg_length(argv, i);
	}

	// Copie de l'environnement dans le buffer noyau
	for (unsigned int i = 0; i < environnement_lenght; i++)
	{
		memcpy(&kernel_envp[kernel_envp_index], env[i], get_arg_length(env, i));
		kernel_envp_index += get_arg_length(env, i);
	}

	new_context = create_mm_context();

	if (!new_context)
	{
		kfree(kernel_path);		
		kfree(kernel_argv);
		kfree(kernel_envp);
		return -ERR_NO_MEM;		
	}
	
	// On ne détruit pas tout de suite l'ancien contexte mémoire du processus car
	// on pourrait en avoir besoin en cas d'erreur pour restaurer l'ancien processus
	mm_contexte_SwitchTo(new_context);				// Passe au nouveau contexte mémoire

	stack_end = USER_STACK_VBASE - PHYS_ALIGN_SUP(stack_required_length);

	// On garde la même pile noyau et les descripteurs de fichiers ouverts
	// Par contre on remappe la pile utilisateur
	PagingAutoMap(stack_end, ((USER_STACK_VBASE - stack_end) / PAGE_SIZE) + 1, VM_MAP_USER | VM_MAP_WRITE);
	cpu->esp3 = USER_STACK_VBASE;
	esp = (char *)cpu->esp3;

	esp -= PTR_SIZE;
	memset(esp, 0, 4);										// On met un pointeur NULL tout en haut de la pile

	// On copie d'abord l'environnement
	if (has_environnement)
	{
		esp -= environnement_size;
		memcpy(esp, kernel_envp, environnement_size);
	}

	stack_envp_data_start = (vaddr_t)esp;

	// Pointeur NULL pour signaler la fin des arguments
	esp -= PTR_SIZE;
	memset(esp, 0, 4);

	// Puis on copie les arguments
	if (has_argument)
	{
		esp -= argument_size;
		memcpy(esp, kernel_argv, argument_size);
	}

	stack_argv_data_start = (vaddr_t)esp;

	// Pointeur NULL pour indiquer la fin du tableau de pointeur
	esp -= PTR_SIZE;
	memset(esp, 0, 4);

	// On fabrique le tableau de pointeur si besoin est
	if (has_environnement)
	{
		uint32_t environnement_dealed_size = 0;
		esp -= environnement_lenght * PTR_SIZE;

		for (unsigned int i = 0; i < environnement_lenght; i++)
		{
			vaddr_t arg_addr = &kernel_envp[environnement_dealed_size] - &kernel_envp[0] + stack_envp_data_start;

			memcpy(esp, &arg_addr, PTR_SIZE);
			esp += PTR_SIZE;
			environnement_dealed_size += strlen(&kernel_envp[environnement_dealed_size]) + 1;
		}

		esp -= environnement_lenght * PTR_SIZE;
		stack_envp_start = (vaddr_t)esp;
	}

	// Pointeur NULL pour indiquer la fin du tableau de pointeur
	esp -= PTR_SIZE;
	memset(esp, 0, 4);

	// On fabrique le tableau de pointeur s'il le faut
	if (has_argument)
	{
		uint32_t argument_dealed_size = 0;
		esp -= argument_length * PTR_SIZE;

		for (unsigned int i = 0; i < argument_length; i++)
		{
			vaddr_t arg_addr = &kernel_argv[argument_dealed_size] - &kernel_argv[0] + stack_argv_data_start;

			memcpy(esp, &arg_addr, PTR_SIZE);
			esp += PTR_SIZE;
			argument_dealed_size += strlen(&kernel_argv[argument_dealed_size]) + 1;
		}

		esp -= argument_length * PTR_SIZE;
		stack_argv_start = (vaddr_t)esp;
	}

	// Pointeur vers le tableau de pointeur
	esp -= PTR_SIZE;
	memcpy(esp, &stack_envp_start, PTR_SIZE);

	// Pointeur vers le tableau de pointeur
	esp -= PTR_SIZE;
	memcpy(esp, &stack_argv_start, PTR_SIZE);

	// Nombre d'arguments
	esp -= PTR_SIZE;						// Taille d'un entier
	memcpy(esp, &argument_length, 4);		// Copie le nombre d'arguments

	esp -= PTR_SIZE;
	memset(esp, 0, 4);	
		
	// Puis on actualise ESP
	cpu->esp3 = (uint32_t)esp;

	// On charge le nouveau processus pr son chemin d'accès
	ret_code = parse_path(kernel_path, &parsed_path);

	if(ret_code != RET_OK)
		goto load_failed;

	ret_code = LoadElfFile(kernel_path, &current->cpu_contexte->eip, current);
	kfree(parsed_path);
	
	if(ret_code == RET_OK)
		goto terminate_initialization;

	// On modifie kernel_path en fonction de la variable PATH
	if(environnement_lenght != 0 && relative_path == TRUE && kernel_path[0] != '.')
	{
		char **new_env = (char **)stack_envp_start;
		int kernel_path_size = strlen(kernel_path);
		int offset_in_env = 11;

		for(unsigned int i = 0; i < environnement_lenght; i++)
		{
			if(strncmp(new_env[i], "HAOUD_PATH=", 10) == 0)
			{
				int var_env_path_len = strlen(new_env[i]);

				while(offset_in_env < var_env_path_len)
				{
					char *start_of_path = (char *) ((uint32_t)(new_env[i]) + offset_in_env);
					char *end_of_path = strchr(start_of_path, ';');
					char *path_concat;

					if(end_of_path == NULL)
						goto load_failed;

					*end_of_path = '\0';			// Remplace le ; par un caractère nul

					path_concat = kmalloc(kernel_path_size + ((uint32_t)end_of_path - (uint32_t)start_of_path) + 1);
					strcpy(path_concat, start_of_path);
					strcat(path_concat, kernel_path);

					ret_code = parse_path(path_concat, &parsed_path);

					if(ret_code != RET_OK)
					{
						offset_in_env += strlen(start_of_path) + 1;
						kfree(path_concat);
						*end_of_path = ';';			// Remet le ;
						continue;
					}

					ret_code = LoadElfFile(parsed_path, &current->cpu_contexte->eip, current);

					if(ret_code != RET_OK && ret_code != -ERR_NOT_FOUND)
						goto load_failed;

					kfree(path_concat);
					kfree(parsed_path);

					if(ret_code == RET_OK)
					{
						*end_of_path = ';';				// Remet le ;
						goto terminate_initialization;
					}

					offset_in_env += strlen(start_of_path) + 1;
					*end_of_path = ';';				// Remet le ;
				}
			}
		}
	}

	goto load_failed;
	
terminate_initialization:
	init_brk(current);
	init_signals(current);

	// Taille de la pile par défaut
	current->stack_start = (void *)USER_STACK_VBASE;
	current->stack_end = (void *)(stack_end & PAGE_MASK);

	current->fpu_state_loaded = FALSE;
	current->used_fpu = FALSE;

	// On actualise la structure des registres
	current->cpu_contexte = (struct cpu_state *)cpu;
	init_cwd(current, parsed_path);
	// Pas besoin de mettre à jour l'arbre des processus

	mm_context_unref(current->mm_contexte);
	current->mm_contexte = new_context;
	kfree(parsed_path);
	kfree(kernel_path);
	kfree(kernel_argv);
	kfree(kernel_envp);

	debugk("[EXEC]: Loading OK\n");
	
	return RET_OK;

load_failed:
	parse_path(kernel_path, &parsed_path);
	debugk("[EXEC]: Failed to load file %s\n", parsed_path);
	mm_contexte_SwitchTo(current->mm_contexte);
	mm_context_unref(new_context);
	kfree(parsed_path);
	kfree(kernel_path);		
	kfree(kernel_argv);
	kfree(kernel_envp);

	cpu->esp3 = saved_esp;
	return ret_code;	
}
