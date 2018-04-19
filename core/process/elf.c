#include <types.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/mm/heap.h>
#include <core/fs/file.h>
#include <core/fs/inode.h>
#include <core/process/elf.h>

bool_t is_elf_file(char *signature)
{
	if(signature[0] == 0x7F && signature[1] == 'E' && signature[2] == 'L' && signature[3] == 'F')
		return TRUE;

	return FALSE;
}

/*
 * Cette fonction charge un fichier éxécutable PE (.exe sous Windows) et
 * remplace le contexte d'éxécution du processus appelant avec le code et
 * les données du fichier chargé
 */
hret_t LoadElfFile(const char *pathname, uint32_t *eip, struct process *p)
{
	hret_t ret;							// Code de retour
	size_t file_size;					// Taille du fichier executable
	char *file_buffer;					// Buffer du fichier chargé entièrement en mémoire
	char *remaning_path;				// Si on ne peut pas trouver l'inode (chemin d'accès non résolu)
	size_t saved_file_size;				// Taille de la lecture
	uint32_t addr_end_of_file = 0;		//  Section la + lointaine en mémoire	
	uint32_t addr_start_of_file = 0xFFFFFFFF;	// Section la + proche en mémoire	

	struct inode *i_file;				// Inode du fichier correspondant à pathname
	struct file *open_file;				// Fichier ouvert de l'inode correspondant à pathname

	struct ElfHeader *elf_header;
	struct ElfProgramHeader *program_header;

	// On recherche l'inode du fichier à charger
	ret = lookup_inode(pathname, &remaning_path, &i_file);

	if (ret != RET_OK)
		return ret;
		
	// Si le chemin d'accès n'a pas été résolu à 100 %
	if (remaning_path != NULL)
	{
		if (i_file != NULL)
			unref_inode(i_file);

		kfree(remaning_path);
		return -ERR_NOT_FOUND;
	}
	
	// On vérifie que l'inode est bien un fichier
	if (i_file->type != INODE_FILE)
	{
		unref_inode(i_file);
		return -ERR_NOT_FILE;
	}
	
	// On ouvre le fichier
	ret = new_opened_file(&process_list[0], i_file, OPEN_READ, &open_file);			// Fichier ouvert au nom du processus idle
	if (ret != RET_OK)
	{
		unref_inode(i_file);
		return ret;
	}

	// Obtient la taille du fichier
	file_size = get_open_file_size(open_file);

	// On alloue le buffer
	file_buffer = kmalloc(file_size);
	if (!file_buffer)
	{
		unref_open_file(open_file);
		unref_inode(i_file);

		return -ERR_NO_MEM;
	}

	// On lit les données en mémoire
	saved_file_size = file_size;
	open_file->file_op->read(open_file, file_buffer, &file_size);

	// Si le fichier n'a pas été entièrement lu
	if (file_size != saved_file_size)
	{
		debugk("ELF: incomplet reading (%u/%u bytes readed)\n", file_size, saved_file_size);
		unref_open_file(open_file);
		unref_inode(i_file);
		kfree(file_buffer);

		return -ERR_UNKNOW;
	}

	elf_header = (struct ElfHeader *)file_buffer;
	if(!is_elf_file((char *)elf_header->signature))
	{
		debugk("[ELF]: Trying to load a non elf file\n");
		unref_open_file(open_file);
		unref_inode(i_file);
		kfree(file_buffer);

		return -ERR_BAD_ARG;
	}

	*eip = elf_header->entry;
	program_header = (struct ElfProgramHeader *)(file_buffer + elf_header->program_table_off);

	for(int i = 0; i < elf_header->program_table_nb_entries; i++, program_header++)
	{
		if(program_header->type != PT_LOAD)
			continue;

		if(program_header->vaddr < (uint32_t)addr_start_of_file)
			addr_start_of_file = program_header->vaddr;

		if(program_header->vaddr +  program_header->mem_size > (uint32_t)addr_end_of_file)
			addr_end_of_file = program_header->vaddr +  program_header->mem_size;

		PagingAutoMap(program_header->vaddr, (program_header->mem_size / PAGE_SIZE) + 1, VM_MAP_USER | VM_MAP_WRITE | VM_MAP_ATOMIC);

		memcpy((void*)program_header->vaddr, file_buffer + program_header->offset, program_header->file_size);
		memset((void *)(program_header->vaddr + program_header->file_size), 0, program_header->mem_size - program_header->file_size);
	}

	// On ferme le fichier car les données ont été lues
	unref_open_file(open_file);
	unref_inode(i_file);

	p->code_start = (void *)addr_start_of_file;
	p->code_end = (void *)addr_end_of_file;

	kfree(file_buffer);
	return RET_OK;
}
