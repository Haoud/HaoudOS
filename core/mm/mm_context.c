#include <assert.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/mm/heap.h>
#include <core/mm/area.h>
#include <core/mm/mm_context.h>

struct mm_context *current_mm_context;			// Le contexte m�moire actuel
static struct mm_context *mm_context_list;		// La liste de tous les contextes m�moires

/*
 * Cette fonction initialise le contexte m�moire du noyau
 */
void Setup_MMContext_Kernel(void)
{
	struct mm_context *context_kernel;
	context_kernel = kmalloc(MM_CONTEXT_STRUCT_SIZE);

	if (!context_kernel)
		panic("Erreur lors de l'allocation du contexte m�moire du noyau");

	context_kernel->pd_paddr = get_current_pd_paddr();		// Adresse physique du r�pertoire de page du noyau
	context_kernel->pd_vaddr = get_vm_area(1, 0);			// Adresse virtuelle o� sera mapp� le r�pertoire de page du noyau

	if (!context_kernel->pd_vaddr)
		panic("Erreur lors de l'obtention d'une plage m�moire dans l'espace d'adressage du noyau");

	PagingMap(context_kernel->pd_paddr, context_kernel->pd_vaddr, VM_MAP_WRITE);

	list_init(mm_context_list);
	list_singleton(mm_context_list, context_kernel);

	context_kernel->ref = 2;				// Pour �viter la lib�ration de cette structure lors d'une commutation de tache
	current_mm_context = context_kernel;	// Le contexte m�moire actuel
}

/*
 * Cette fonction permet de cr�er un nouveau contexte m�moire: l'espace d'adresse du noyau,
 * identique � tous les processus, est copi� dans ce nouveau contexte. L'espace d'adressage
 * du processus est, pour l'intant, vide.
 */
struct mm_context *create_mm_context(void)
{
	struct mm_context *new_mm_context;
	new_mm_context = kmalloc(MM_CONTEXT_STRUCT_SIZE);

	if (!new_mm_context)
	{
		debugk("Unable to allocate new memory context !!\n");
		return NULL;
	}

	new_mm_context->pd_vaddr = get_vm_area(1, 0);
	if (!new_mm_context->pd_vaddr)
	{
		debugk("create_mm_context(): Unable to get virtual memory area !!\n");
		kfree(new_mm_context);

		return NULL;
	}

	// On map l'adresse virtuelle du r�pertoire de page en m�moire
	paddr_t paddr = GetFreePhysicalPage();
	PagingMap(paddr, new_mm_context->pd_vaddr, VM_MAP_WRITE);
	UnrefPhysicalPage(paddr);

	new_mm_context->pd_paddr = PagingGetPhysicalAddr(new_mm_context->pd_vaddr);
	if (!new_mm_context->pd_paddr)
	{
		debugk("create_mm_context(): Unable to get physical page !!\n");
		FreeVmArea(new_mm_context->pd_vaddr);
		kfree(new_mm_context);

		return NULL;
	}

	/* Copie l'espace d'adressage du noyan dans ce nouveau contexte m�moire*/
	PagingCopyKernelSpace(new_mm_context->pd_vaddr, current_mm_context->pd_vaddr);

	new_mm_context->ref = 1;
	list_add_after(mm_context_list, new_mm_context);

	return new_mm_context;
}

/*
 * Cette fonction permet de copier enti�rement le contexte m�moire pass� en argument et
 * retourne le nouveau contexte m�moire ainsi cr�e qui est identique au contexte m�moire
 * pass� en argument
 *
 * ATTENTION: Cette fonction copie TOUT l'espace d'adressage du processus, ce qui est tr�s 
 * lourd (si le porcessus fait 1GO alors 1GO de m�moiure va �tre copi�), tr�s mal optimis�,
 * surtout au niveau des caches et enfin, parfois inutile (g�n�ralement, cette fonction est
 * utilis� lors des fork, et apr�s un fork, un exec est g�n�ralement fait, ce qui d�truit 
 * notre precieux espace d'adressage copi�)
 *
 * TODO: Impl�menter le COW (Copy On Write, Copie � l'�criture) qui permet la copie de la page
 * seulement lorsque n�cessaire mais implique un l�ger surcout � cause de la faute de page ainsi
 * g�n�r�: le syst�me est plus rapide mais (un peu) moins r�actif
 */
struct mm_context *mm_context_duplicate(struct mm_context *context)
{
	struct mm_context *new_context;
	paddr_t tmp_pt_paddr;						// Page physique temporairement allou� pour la nouvelle table de page
	paddr_t tmp_pg_paddr;						// Page physique temporairement allou� pour la nouvelle page
	uint32_t pd_index;
	uint32_t pt_index;

	pde_t *dst_pd;									
	pte_t *dst_pt;	
	uint32_t *dst_pg;

	pde_t *src_pd;									
	pte_t *src_pt;		
	uint32_t *src_pg;

	new_context = create_mm_context();			// Cr�er le nouveau contexte m�moire avec la partie noyau synchronis�e

	// Initialise les emplacements m�moires temporaires qui seront mapp�es 
	src_pd = (pde_t *)context->pd_vaddr;
	src_pt = (pde_t *)get_vm_area(1, 0);
	src_pg = (uint32_t *)get_vm_area(1, 0);

	dst_pd = (pde_t *)new_context->pd_vaddr;
	dst_pt = (pde_t *)get_vm_area(1, 0);
	dst_pg = (uint32_t *)get_vm_area(1, 0);

	if (!new_context)
		return NULL;

	// Synchronise manuellement la partie utilisateur (� patir de 1GO, ou 0x40000000)
	for (pd_index = pd_offset(0x40000000); pd_index < PD_NR_ENTRIES; pd_index++)
	{
		// Copie l'entr�e du r�pertoire de page
		dst_pd[pd_index] = src_pd[pd_index];

		// Si la table de page n'est pas pr�sente on continue la boucle
		if (!(src_pd[pd_index] & PDE_PRESENT))
			continue;

		// Copie la page de table
		tmp_pt_paddr = GetFreePhysicalPage();

		// On mappe en m�moire la page de table source et destination
		PagingMap((paddr_t)(src_pd[pd_index] & PAGE_MASK), (vaddr_t)src_pt, VM_MAP_WRITE);
		PagingMap(tmp_pt_paddr, (vaddr_t)dst_pt, VM_MAP_WRITE);

		// Copie la page de table
		for (pt_index = 0; pt_index < PT_NR_ENTRIES; pt_index++)
		{
			dst_pt[pt_index] = src_pt[pt_index];

			// Si la table de page n'est pas pr�sente on continue la boucle
			if (!(src_pt[pt_index] & PTE_PRESENT))
				continue;

			// Ici, il faut copier la page physique en m�moire, on la mappe donc
			tmp_pg_paddr = GetFreePhysicalPage();

			PagingMap((paddr_t)(src_pt[pt_index] & PAGE_MASK), (vaddr_t)src_pg, VM_MAP_WRITE);
			PagingMap((vaddr_t)tmp_pg_paddr, (vaddr_t)dst_pg, VM_MAP_WRITE);

			memcpy((char *)dst_pg, (char *)src_pg, PAGE_SIZE);		// Copie la page

			// Demappe la page source et destination
			PagingUnmap((vaddr_t)src_pg);
			PagingUnmap((vaddr_t)dst_pg);

			// Actualise l'entr�e de page destination
			dst_pt[pt_index] = (tmp_pg_paddr & PAGE_MASK) | (dst_pt[pt_index] & (~PAGE_MASK));
			tmp_pg_paddr = 0;

			// On indique que la page de table est utilis� un fois de plus
			RefPhysicalPage(tmp_pt_paddr);	
		}

		// On d�mmape les pages de tables
		PagingUnmap((vaddr_t)src_pt);
		PagingUnmap((vaddr_t)dst_pt);

		// Actualise l'entr�e du r�pertoire de page
		dst_pd[pd_index] = (tmp_pt_paddr & PAGE_MASK) | (dst_pd[pd_index] & (~PAGE_MASK));
		tmp_pt_paddr = 0;
	}

	// Lib�re les emplacements temporaires
	FreeVmArea((vaddr_t)dst_pt);
	FreeVmArea((vaddr_t)dst_pg);
	FreeVmArea((vaddr_t)src_pt);
	FreeVmArea((vaddr_t)src_pg);

	return new_context;				// Retourne le contexte m�moire entri�rement copi�
}

/*
 * Cette fonction permet de changer de contexte m�moire
 *
 * NOTE: Cette fonction vide implicitement le cache TLB ainsi que d'autres caches
 */
void mm_contexte_SwitchTo(struct mm_context *to)
{
	struct mm_context *old_mm_context;

	AssertFatal(to != NULL);
	AssertFatal(to->ref != 0);

	/* Pour �viter les changements de contexte m�moire inutile qui souille le cache */
	if (to != current_mm_context)
	{
		old_mm_context = current_mm_context;
		SetCurrentPD(to->pd_paddr);

		//FIXME: D�sactiver IRQs

		current_mm_context = to;
		mm_context_ref(to);
		mm_context_unref(old_mm_context);

		//FIXME: R�activer IRQs
	}
}

void mm_context_ref(struct mm_context *context)
{
	AssertFatal(context != NULL);
	AssertFatal(context->ref != 0);

	//FIXME: D�sactiver IRQs
	context->ref++;
	//FIXME: R�activer IRQs
}

void mm_context_unref(struct mm_context *context)
{
	AssertFatal(context != NULL);
	AssertFatal(context->ref != 0);

	//FIXME: D�sactiver IRQs
	context->ref--;
	//FIXME: R�activer IRQs

	if (context->ref > 0)
		return;

	//Ici, le contexte n'est plus utilis�, on peut donc le supprimer
	AssertFatal(context != current_mm_context);

	//TODO: supprimer les pages de l'utilisateur
	DeletePD(context->pd_vaddr);
	list_delete(mm_context_list, context);

	PagingUnmap(context->pd_vaddr);
	FreeVmArea(context->pd_vaddr);
}

/*
 * Cette fonction permet de synchroniser une page de table qui est mapp� dans
 * l'espace d'adressage du noyau dans tous les espaces d'adressages noyau 
 * des autres processus
 */
void sync_pde_in_kernel_space(uint32_t pd_index, uint32_t pde)
{
	struct mm_context *current_mm_item = NULL;
	int nb_element = 0;

	AssertFatal(pd_index <= 1023);

	if (mm_context_list == NULL)
		return;

	list_foreach(mm_context_list, current_mm_item, nb_element)
	{
		uint32_t * dest_pd;

		dest_pd = (uint32_t*)current_mm_item->pd_vaddr;
		dest_pd[pd_index] = pde;
	}
}