#pragma once
#include <types.h>
#include <i386/paging.h>

#define STRUCT_VM_AREA_SIZE		sizeof(struct vm_area)

#define VM_AREA_USED			0x00000001
struct vm_area
{
	vaddr_t base_addr;			// L'adresse de base de la zone virtuelle
	count_t page_count;			// Taille (en pages) de la zoné mémoire virtuelle
	flags_t flags;				// Options de la zone mémoire

	struct vm_area *next;
	struct vm_area *prev;
};

void SetupVmArea(vaddr_t start_vm_area, vaddr_t end_vm_area);

void FreeVmArea(vaddr_t base_area);
vaddr_t get_vm_area(count_t nb_pages, flags_t flags);
