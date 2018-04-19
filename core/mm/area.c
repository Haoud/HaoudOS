#include <lib/list.h>
#include <lib/stdio.h>
#include <core/mm/area.h>
#include <core/mm/heap.h>

static struct vm_area *first_vm_area;

/*
 * Cette fonction initialise le gestionnaire de zone m�moire virtuelles du noyau (ie entre
 * 0 et 0x3FFFFFFF)
 */
void SetupVmArea(vaddr_t start_vm_area, vaddr_t end_vm_area)
{
	struct vm_area *area = kmalloc(sizeof(struct vm_area));

	area->base_addr = start_vm_area;
	area->page_count = (end_vm_area - start_vm_area) / PAGE_SIZE;
	area->flags = 0;

	list_init(first_vm_area);
	list_singleton(first_vm_area, area);
}

/*
 * Cette fonction permet d'obtenir n pages virtuelles contig�es dans l'espace d'adressage
 * du noyau et retourne l'adresse de base de ces pages ou 0 en cas d'erreur (arguments
 * �ronn�s, pas assez de m�moire...)
 * L'adresse virtuelle retourn�e n'est pas mapp�, elle est donc libre pour l'usage de 
 * l'appelant
 */
vaddr_t get_vm_area(count_t nb_pages, flags_t _unused flags)
{
	struct vm_area *current_vm_area;
	struct vm_area *split_vm_area;
	int nb_element = 0;

	if (!nb_pages)
		return 0;

	// Cherche une zone m�moire assez grande
	list_foreach(first_vm_area, current_vm_area, nb_element)
	{
		if (!(current_vm_area->flags & VM_AREA_USED))
			if (current_vm_area->page_count >= nb_pages)
				break;
	}

	// Si on a trouv� la zone
	if (!(current_vm_area->flags & VM_AREA_USED) && (current_vm_area->page_count >= nb_pages))
	{
		// Si la zone n'a pas exactement la bonne taille (tr�s probable)
		if (current_vm_area->page_count > nb_pages)
		{
			// On coupe la zone m�moire en deux
			split_vm_area = kmalloc_no_security(STRUCT_VM_AREA_SIZE);				// On utilise kmalloc_no_security pour �viter une r�cursivit� infinie avec kmalloc

			if (!split_vm_area)
				panic("get_vm_area(): kmalloc_no_security() returned NULL");

			split_vm_area->base_addr = current_vm_area->base_addr + (nb_pages * PAGE_SIZE);
			split_vm_area->page_count = current_vm_area->page_count - nb_pages;

			split_vm_area->flags = 0;

			list_add_after(current_vm_area, split_vm_area);

			current_vm_area->page_count = nb_pages;
		}
		
		set_bits(current_vm_area->flags, VM_AREA_USED);
		return current_vm_area->base_addr;
	}

	return 0;
}

/*
 * Cette fonction lib�re une zone m�moire virtuelle contig�e dans l'espace d'adresse
 * virtuel du noyau
 */
void FreeVmArea(vaddr_t base_area)
{
	struct vm_area *current_vm_area;
	int nb_element = 0;

	// Cherche la zone m�moire associ� � base_area

	list_foreach(first_vm_area, current_vm_area, nb_element)
	{
		if (current_vm_area->base_addr == base_area)
			break;
	}

	if (current_vm_area->base_addr == base_area)
	{
		if (current_vm_area->flags & VM_AREA_USED)
			clear_bits(current_vm_area->flags, VM_AREA_USED);
		else
			panic("Tentative le lib�ration d'une zone m�moire virtuelle lin�aire d�j� libre !!");
	}
	else
		panic("");
	//TODO: Merger les bloc libres
}