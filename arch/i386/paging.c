/*
 * This file was created on Tue Mar 27 2018
 * Copyright 2018 Romain CADILHAC
 *
 * This file is a part of HaoudOS.
 *
 * HaoudOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HaoudOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with HaoudOS. If not, see <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <i386/paging.h>
#include <core/mm/area.h>
#include <core/mm/mm_context.h>
#include <driver/bochs/bochs.h>

/*
 * Cette fonction permet d'initialise la pagination: elle se contente de mapper � l'identique 
 * (dans l'espace d'adressage du noyau uniquement) les portions de m�moire indispensable au
 * noyau comme son code, sa pile, le BIOS, la m�moire vid�o...
 */
void SetupPaging(void)
{
	static pde_t *pd0 = NULL;
	pd0 = (pde_t *)GetFreePhysicalPage();					// Allocation du r�pertoire de page
	clear_pd(pd0);											// Initialisation du r�pertoire de page

	// Mapping pour la pile du noyau
	for (paddr_t paddr = 0x88000; paddr < 0x90000; paddr += PAGE_SIZE)
		PagingMapHelper(pd0, paddr, paddr);

	// Mapping pour le BIOS et la m�moire vid�o
	for (paddr_t paddr = 0xA0000; paddr < 0x100000; paddr += PAGE_SIZE)
		PagingMapHelper(pd0, paddr, paddr);

	// Mapping pour le noyau + liste des pages physique libre
	for (paddr_t paddr = 0x100000; paddr < PHYS_ALIGN_SUP(GetEndOfMemMap()); paddr += PAGE_SIZE)
		PagingMapHelper(pd0, paddr, paddr);
	
	// Mise en place du Mirroring
	pd0[pd_offset(MIRRORING_ADDR)] = ((paddr_t)pd0 & PAGE_MASK);
	pd0[pd_offset(MIRRORING_ADDR)] |= (PTE_PRESENT | PDE_WRITABLE);

	SetCurrentPD((vaddr_t)pd0);		// On utilise le r�pertoire de page du noyau
	EnablePaging();					// <--- La pagination est r�element activ� ICI

	// Tout acc�s en dehors des zones m�moire d�finient ci-dessus condira � une
	// faute de page (sauf si la page est mapp�e gr�ce � Paging): crash du noyau
}

/*
 * Cette fonction tr�s simple se charge d'activer la pagination
 * (modification du registre cr0) + charge l'option permettant
 * de conserver les droits d'acc�s en mode noyau sur des pages
 * utilisateur
 *
 * NOTE: un r�pertoire de page initialis� doit �tre pr�sent dans
 * le registre cr3 pour �viter la triple faute
 */
void EnablePaging(void)
{
	asm("mov eax, cr0			\n\
		 or eax, %0				\n\
		 or eax, %1				\n\
		 mov cr0, eax" :: "i"(CR0_PAGING_BIT), "i"(CR0_WRITE_PROTECT_BIT) : "eax");
}

/*
 * Cette fonction d�sactive la pagination, mais attention!!! 
 * D�sactiver la pagination sans raison ni pr�paration conduira
 * � un reboot de la machine
 */
void DisablePaging(void)
{
	asm("mov eax, cr0					\n\
		and eax, %0			\n\
		mov cr0, eax" :: "i"(~CR0_PAGING_BIT) : "eax");
}

/*
 * Cette fonction permet d'invalider le contenu du TLB associ� � une
 * adresse virtuelle. Cette fonction est utile si les pages de traductions
 * sont modifi�s: le contenu du cache n'est pas automatiquement mit � jour
 * et peut donc corrompre les calculs en m�moire
 */
void invlpg(vaddr_t addr)
{
	asm volatile("invlpg [eax]" :: "a"(addr));
}

/*
 * Cette fonction vide enti�rement le cache TLB de la MMU: cette fonction doit
 * �tre utilis� avec parcimonie car l'execution du noyau (ou processus) est plus
 * lente apr�s cette fonction
 */
void Flush_TLB(void)
{
	asm("mov eax, cr3	\n\
		 mov cr3, eax");
}

/*
 * Cette fonction permet de changer le r�pertoire de page courant utilis� par
 * la MMU. Ce changement provoque l'invalidation de tout le cache TLB
 */
void SetCurrentPD(vaddr_t pd_addr)
{
	asm("mov eax, %0	\n\
	     mov cr3, eax" :: "m"(pd_addr) : "eax");
}

/*
 * Cette fonction permet de retourner l'adresse PHYSIQUE du r�pertoire de page
 * actuel charg� dans cr3
 */
paddr_t GetCurrentPD(void)
{
	paddr_t __cr3_value;
	asm("mov eax, cr3	\n\
		 mov %0, eax" : "=m"(__cr3_value) :: "eax");
	return __cr3_value;
}

/*
 * Cette fonction permet d'aider � l'initialisation du la pagination en �tablissant
 * un mapping simple: comme on est encore en mode non-pagin�, on peut directement 
 * acc�der aux r�pertoires de pages et aux tables de pages directement avec leurs
 * adresses physiques
 */
void PagingMapHelper(pde_t *pd, paddr_t paddr, vaddr_t vaddr)
{
	unsigned int pd_index = pd_offset(vaddr);
	unsigned int pt_index = pt_offset(vaddr);
	pte_t *pt = NULL;

	if (pde_test_flag(pd[pd_index], PDE_PRESENT))
	{
		pt = (pte_t *)(pd[pd_index] & PAGE_MASK);

		/* Normalement, la page correspondante n'est pas pr�sente en m�moire physique
		 * car cette fonction est la premi�re � initialiser la page et ne doit pas 
		 * d�j� �tre initialis� (probl�me dans le code sinon...)
		 */
		if (!pte_test_flag(pt[pt_index], PTE_PRESENT))
			RefPhysicalPage((paddr_t)pt);
		else
			panic("Erreur lors de l'aide au mapping: page d�j� pr�sente");

	}
	else
	{
		// Si le r�pertoire de page n'existe pas, on le cr�e
		pt = (pte_t *)GetFreePhysicalPage();
		AssertFatal(pt != NULL);

		clear_pt(pt);

		// On actualise le r�pertoire de page
		pd[pd_index] = ((paddr_t)pt & PAGE_MASK);					// Adresse de base de la page de table
		pde_set_flags(pd[pd_index], PDE_PRESENT | PDE_WRITABLE);	// Option du r�pertoire de page
	}

	pt[pt_index] = (paddr & PAGE_MASK);								// Adresse de base de la page physique
	pte_set_flags(pt[pt_index], PTE_PRESENT | PTE_WRITABLE);		// Option de la page de table
}

/*
 * Cette fonction permet de d�mapper une adresse virtuelle. Si l'adresse virtuelle est situ�
 * dans l'espace d'adresse du noyau (ie < 0x40000000), alors tout les l'espace noyau des autres
 * processus est synchronis�
 */
void PagingUnmap(vaddr_t vaddr)
{
	uint32_t pd_index = pd_offset(vaddr);			// Index de l'entr�e du pd correspondant � l'adresse virtuelle vaddr
	uint32_t pt_index = pt_offset(vaddr);			// Index de l'entr�e de la pt correspondante � l'adresse virtuelle vaddr

	pde_t *pd = get_current_pd_vaddr();				// Adresse de base du r�pertoire de page courant
	pte_t *pt = get_pt_vaddr(pd_index);				// Adresse de base de la table de page concern�e

	count_t pt_count = 0;							// Nombre de fois que la page de table est utilis�

	if (!pde_test_flag(pd[pd_index], PDE_PRESENT))	// Si la table de page n'est pas pr�sent
		return;
	if (!pte_test_flag(pt[pt_index], PTE_PRESENT))	// Si la page n'est pas pr�sent
		return;

	// On ne peut pas d�mapper � l'emplacement du mirroring
	if (vaddr >= MIRRORING_ADDR && vaddr < (MIRRORING_ADDR + MIRRORING_SIZE))
		return;

	UnrefPhysicalPage(pt[pt_index] & PAGE_MASK);	// On indique que la page physique n'est plus utilis�
	pt[pt_index] = 0;								// On r�initialise l'entr�e de la table de page
	invlpg(vaddr);									// Invalide la cache TLB pour �viter des erreurs de traductions

	pt_count = UnrefPhysicalPage(pd[pd_index] & PAGE_MASK);		// On d�cr�mente le compteur d'utilisation de l'adresse physique de la table de page

	// Si la page de table n'est plus utile
	if (pt_count == 0)		
	{
		pd[pd_index] = 0;							// On supprime l'entr�e correspondante dans le r�pertoire de page

		if (vaddr < MIRRORING_ADDR)
			sync_pde_in_kernel_space(pd_index, pd[pd_index]);

		invlpg((vaddr_t)pt);						// On invalide la cache associ� � la table de page
	}
}

/*
 * Cette fonction prend comme param�tre l'adresse VIRTUELLE d'un r�pertoire de page
 * (qui doit donc �tre mapp�) et lib�re toute les pages �tant dans l'espace d'adressage
 * de l'utilisateur
 */
void DeletePD(vaddr_t pd_vaddr)
{
	uint32_t pd_index = 0;							// Index de l'entr�e du pd correspondant � l'adresse virtuelle vaddr
	uint32_t pt_index = 0;							// Index de l'entr�e de la pt correspondante � l'adresse virtuelle vaddr

	pde_t *pd = (pde_t *)pd_vaddr;					// Adresse de base du r�pertoire de page courant
	pte_t *pt = (pte_t *)get_vm_area(1, 0);			// Adresse de base de la table de page concern�e
	
	/*
	 * Comme l'espace d'adressage du noyau est commun � tous les processus on ne d�mmape
	 * pas cette partie l�: on commence donc � partir de l'espace d'adressage utilisateur
	 */
	for (pd_index = pd_offset(0x40000000); pd_index < PD_NR_ENTRIES; pd_index++)
	{
		if (!(pd[pd_index] & PDE_PRESENT))
			continue;
		
		PagingMap(pd[pd_index] & PAGE_MASK, (vaddr_t)pt, VM_MAP_WRITE);		// Mappe temporairement la table de page
		
			for (pt_index = 0; pt_index < PT_NR_ENTRIES; pt_index++)
			{
				if (!(pt[pt_index] & PTE_PRESENT))
					continue;

				UnrefPhysicalPage(pt[pt_index] & PAGE_MASK);				// On d�mmape la page
				UnrefPhysicalPage(pd[pd_index] & PAGE_MASK);				// Et on d�cr�mente le compteur d'utilisation de la table de page
			}

		PagingUnmap((vaddr_t)pt);											// On d�mmape la page de table mapp�e temporairement
	}
	FreeVmArea((vaddr_t)pt);
}

/*
 * Cette fonction retourne l'adresse physique d'une adresse virtuelle UNIQUEMENT
 * si l'adresse virtuelle est mapp�e en m�moire
 */
paddr_t PagingGetPhysicalAddr(vaddr_t vaddr)
{
	uint32_t pd_index = pd_offset(vaddr);			// Index de l'entr�e du pd correspondant � l'adresse virtuelle vaddr
	uint32_t pt_index = pt_offset(vaddr);			// Index de l'entr�e de la pt correspondante � l'adresse virtuelle vaddr
	uint32_t pg_offset = pg_offset(vaddr);			// D�calage interne dans la page

	pde_t *pd = get_current_pd_vaddr();				// Adresse de base du r�pertoire de page courant
	pte_t *pt = get_pt_vaddr(pd_index);				// Adresse de base de la table de page concern�e

	/* Si la table de page n'est pas pr�sente, on returne 0 (erreur) */
	if (!pde_test_flag(pd[pd_index], PDE_PRESENT))
		return 0;

	/* Si la page n'est pas pr�sente, on returne 0 */
	if (!pte_test_flag(pt[pt_index], PTE_PRESENT))
		return 0;

	return (paddr_t) ((pt[pt_index] & PAGE_MASK) + pg_offset);		// Adresse physique de la page + d�calage dans la page
} 

/*
 * Cette fonction permet de mapper une page physique � une adresse virtuelle avec les
 * droits d'acc�s sp�cifi� par l'argument flags
 * Note: Si vous voulez mapper un grand intervalle d'adresses virtuelles continues et que 
 * les pages physiques associ�es aux adresses virtuelles importe peu, je vous conseille
 * d'utiliser PagingAutoMap bas�e sur cette fonction mais plus pratique
 */
void PagingMap(paddr_t paddr, vaddr_t vaddr, map_flags_t flags)
{
	uint32_t pd_index = pd_offset(vaddr);		// Index de l'entr�e du pd correspondant � l'adresse virtuelle vaddr
	uint32_t pt_index = pt_offset(vaddr);		// Index de l'entr�e de la pt correspondante � l'adresse virtuelle vaddr

	pde_t *pd = get_current_pd_vaddr();			// Adresse de base du r�pertoire de page courant
	pte_t *pt = get_pt_vaddr(pd_index);			// Adresse de base de la table de page concern�e

	// On ne peut pas mapper � l'emplacement du mirroring
	if (vaddr >= MIRRORING_ADDR && vaddr < (MIRRORING_ADDR + MIRRORING_SIZE))
		return;

	// Si la table de page n'existe pas, on en alloue une
	if (!pde_test_flag(pd[pd_index], PDE_PRESENT))
	{
		paddr_t pt_allocated = GetFreePhysicalPage();

		if (!pt_allocated)								// Si pas assez de m�moire on abandonne la mapping
			return;

		pd[pd_index] = pt_allocated;					// Adresse de la table de page allou�
		pde_set_flags(pd[pd_index], PDE_PRESENT);		// Page pr�sente en m�moire

		if (flags & VM_MAP_USER)
			pde_set_flags(pd[pd_index], PDE_USER);
		if (flags & VM_MAP_WRITE)
			pde_set_flags(pd[pd_index], PDE_WRITABLE);

		if (vaddr < MIRRORING_ADDR)
		{
			pde_clear_flags(pd[pd_index], PDE_USER);
			sync_pde_in_kernel_space(pd_index, pd[pd_index]);
		}

		invlpg((vaddr_t)pt);		// Invalide le cache par s�curit�
		clear_pt(pt);				// Initialise la page de table � z�ro
	}
	else if (!pte_test_flag(pt[pt_index], PTE_PRESENT))
	{	
		// Si la page n'est pas mapp� alors on incr�mente le conteur d'utilisation de la page de table
		// (Situation normal de mapping)
		RefPhysicalPage(pd[pd_index] & PAGE_MASK);
	}
	else
	{
		// Erreur, page d�j� mapp�, par s�curit� on fait crasher le noyau
		// M�me si on pourrait automatiquement d�mmaper l'ancienne ressource
		// mais je pr�f�re tout faire crasher pour �viter corruption des donn�es
		panic("Tentative de mapping � une adresse virtuelle d�j� utilis�e: 0x%08x", vaddr);
	}

	pt[pt_index] = paddr;									// Adresse de la table de page allou�
	pte_set_flags(pt[pt_index], PTE_PRESENT);				// Options de la page

	if (flags & VM_MAP_USER)
		pte_set_flags(pt[pt_index], PTE_USER);				
	if (flags & VM_MAP_WRITE)
		pte_set_flags(pt[pt_index], PTE_WRITABLE);

	RefPhysicalPage(paddr);
	invlpg(vaddr);
}

/*
 * Cette fonction permet de copier l'espace d'adresse du noyau dans un nouvel
 * espace d'adressage.
 * NOTE: Toutes les entr�es du r�pertoire de pages sont effac�es, donc faites
 * attention !!
 */
void PagingCopyKernelSpace(vaddr_t dst_pd, vaddr_t src_pd)
{
	vaddr_t *dst_pd_ptr = (vaddr_t *)dst_pd;
	vaddr_t *src_pd_ptr = (vaddr_t *)src_pd;
	clear_pd(dst_pd_ptr);

	paddr_t dst_paddr_of_pd = PagingGetPhysicalAddr(dst_pd);

	// Copie des pages communes � tous les processus (le noyau)
	for (unsigned int pd_index = 0; pd_index < pd_offset(MIRRORING_ADDR); pd_index++)
		dst_pd_ptr[pd_index] = src_pd_ptr[pd_index];
	
	// Mise en place du mirroring (diff�rent pour chaque contexte m�moire)
	dst_pd_ptr[pd_offset(MIRRORING_ADDR)] = dst_paddr_of_pd & PAGE_MASK;		// Pour pouvoir modifer le pd ult�rieurement
	dst_pd_ptr[pd_offset(MIRRORING_ADDR)] |= PDE_PRESENT | PDE_WRITABLE;		// Option du mirroring
}

/*
 * Cette fonction est semblable � PagingMap sauf que les pages physiques sont prise au hasard
 * pour la mapping et que cette fonction s'avv�re tr�s utile pour mapper un intervalle d'adresses
 * virtuelles simplement
 */
void PagingAutoMap(vaddr_t vaddr_start, count_t nb_pages, map_flags_t flags)
{
	paddr_t tmp_paddr;
	
	for (unsigned int i = 0; i < nb_pages; i++)
	{
		tmp_paddr = GetFreePhysicalPage();
		if (!tmp_paddr)
			panic("PagingAutoMap(): Pas assez de m�moire physique");

		PagingMap(tmp_paddr, vaddr_start + (i * PAGE_SIZE), flags);
		UnrefPhysicalPage(tmp_paddr);
	}
}