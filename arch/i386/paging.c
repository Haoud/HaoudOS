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
 * Cette fonction permet d'initialise la pagination: elle se contente de mapper à l'identique 
 * (dans l'espace d'adressage du noyau uniquement) les portions de mémoire indispensable au
 * noyau comme son code, sa pile, le BIOS, la mémoire vidéo...
 */
void SetupPaging(void)
{
	static pde_t *pd0 = NULL;
	pd0 = (pde_t *)GetFreePhysicalPage();					// Allocation du répertoire de page
	clear_pd(pd0);											// Initialisation du répertoire de page

	// Mapping pour la pile du noyau
	for (paddr_t paddr = 0x88000; paddr < 0x90000; paddr += PAGE_SIZE)
		PagingMapHelper(pd0, paddr, paddr);

	// Mapping pour le BIOS et la mémoire vidéo
	for (paddr_t paddr = 0xA0000; paddr < 0x100000; paddr += PAGE_SIZE)
		PagingMapHelper(pd0, paddr, paddr);

	// Mapping pour le noyau + liste des pages physique libre
	for (paddr_t paddr = 0x100000; paddr < PHYS_ALIGN_SUP(GetEndOfMemMap()); paddr += PAGE_SIZE)
		PagingMapHelper(pd0, paddr, paddr);
	
	// Mise en place du Mirroring
	pd0[pd_offset(MIRRORING_ADDR)] = ((paddr_t)pd0 & PAGE_MASK);
	pd0[pd_offset(MIRRORING_ADDR)] |= (PTE_PRESENT | PDE_WRITABLE);

	SetCurrentPD((vaddr_t)pd0);		// On utilise le répertoire de page du noyau
	EnablePaging();					// <--- La pagination est réelement activé ICI

	// Tout accès en dehors des zones mémoire définient ci-dessus condira à une
	// faute de page (sauf si la page est mappée grâce à Paging): crash du noyau
}

/*
 * Cette fonction très simple se charge d'activer la pagination
 * (modification du registre cr0) + charge l'option permettant
 * de conserver les droits d'accès en mode noyau sur des pages
 * utilisateur
 *
 * NOTE: un répertoire de page initialisé doit être présent dans
 * le registre cr3 pour éviter la triple faute
 */
void EnablePaging(void)
{
	asm("mov eax, cr0			\n\
		 or eax, %0				\n\
		 or eax, %1				\n\
		 mov cr0, eax" :: "i"(CR0_PAGING_BIT), "i"(CR0_WRITE_PROTECT_BIT) : "eax");
}

/*
 * Cette fonction désactive la pagination, mais attention!!! 
 * Désactiver la pagination sans raison ni préparation conduira
 * à un reboot de la machine
 */
void DisablePaging(void)
{
	asm("mov eax, cr0					\n\
		and eax, %0			\n\
		mov cr0, eax" :: "i"(~CR0_PAGING_BIT) : "eax");
}

/*
 * Cette fonction permet d'invalider le contenu du TLB associé à une
 * adresse virtuelle. Cette fonction est utile si les pages de traductions
 * sont modifiés: le contenu du cache n'est pas automatiquement mit à jour
 * et peut donc corrompre les calculs en mémoire
 */
void invlpg(vaddr_t addr)
{
	asm volatile("invlpg [eax]" :: "a"(addr));
}

/*
 * Cette fonction vide entièrement le cache TLB de la MMU: cette fonction doit
 * être utilisé avec parcimonie car l'execution du noyau (ou processus) est plus
 * lente après cette fonction
 */
void Flush_TLB(void)
{
	asm("mov eax, cr3	\n\
		 mov cr3, eax");
}

/*
 * Cette fonction permet de changer le répertoire de page courant utilisé par
 * la MMU. Ce changement provoque l'invalidation de tout le cache TLB
 */
void SetCurrentPD(vaddr_t pd_addr)
{
	asm("mov eax, %0	\n\
	     mov cr3, eax" :: "m"(pd_addr) : "eax");
}

/*
 * Cette fonction permet de retourner l'adresse PHYSIQUE du répertoire de page
 * actuel chargé dans cr3
 */
paddr_t GetCurrentPD(void)
{
	paddr_t __cr3_value;
	asm("mov eax, cr3	\n\
		 mov %0, eax" : "=m"(__cr3_value) :: "eax");
	return __cr3_value;
}

/*
 * Cette fonction permet d'aider à l'initialisation du la pagination en établissant
 * un mapping simple: comme on est encore en mode non-paginé, on peut directement 
 * accéder aux répertoires de pages et aux tables de pages directement avec leurs
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

		/* Normalement, la page correspondante n'est pas présente en mémoire physique
		 * car cette fonction est la première à initialiser la page et ne doit pas 
		 * déjà être initialisé (problème dans le code sinon...)
		 */
		if (!pte_test_flag(pt[pt_index], PTE_PRESENT))
			RefPhysicalPage((paddr_t)pt);
		else
			panic("Erreur lors de l'aide au mapping: page déjà présente");

	}
	else
	{
		// Si le répertoire de page n'existe pas, on le crée
		pt = (pte_t *)GetFreePhysicalPage();
		AssertFatal(pt != NULL);

		clear_pt(pt);

		// On actualise le répertoire de page
		pd[pd_index] = ((paddr_t)pt & PAGE_MASK);					// Adresse de base de la page de table
		pde_set_flags(pd[pd_index], PDE_PRESENT | PDE_WRITABLE);	// Option du répertoire de page
	}

	pt[pt_index] = (paddr & PAGE_MASK);								// Adresse de base de la page physique
	pte_set_flags(pt[pt_index], PTE_PRESENT | PTE_WRITABLE);		// Option de la page de table
}

/*
 * Cette fonction permet de démapper une adresse virtuelle. Si l'adresse virtuelle est situé
 * dans l'espace d'adresse du noyau (ie < 0x40000000), alors tout les l'espace noyau des autres
 * processus est synchronisé
 */
void PagingUnmap(vaddr_t vaddr)
{
	uint32_t pd_index = pd_offset(vaddr);			// Index de l'entrée du pd correspondant à l'adresse virtuelle vaddr
	uint32_t pt_index = pt_offset(vaddr);			// Index de l'entrée de la pt correspondante à l'adresse virtuelle vaddr

	pde_t *pd = get_current_pd_vaddr();				// Adresse de base du répertoire de page courant
	pte_t *pt = get_pt_vaddr(pd_index);				// Adresse de base de la table de page concernée

	count_t pt_count = 0;							// Nombre de fois que la page de table est utilisé

	if (!pde_test_flag(pd[pd_index], PDE_PRESENT))	// Si la table de page n'est pas présent
		return;
	if (!pte_test_flag(pt[pt_index], PTE_PRESENT))	// Si la page n'est pas présent
		return;

	// On ne peut pas démapper à l'emplacement du mirroring
	if (vaddr >= MIRRORING_ADDR && vaddr < (MIRRORING_ADDR + MIRRORING_SIZE))
		return;

	UnrefPhysicalPage(pt[pt_index] & PAGE_MASK);	// On indique que la page physique n'est plus utilisé
	pt[pt_index] = 0;								// On réinitialise l'entrée de la table de page
	invlpg(vaddr);									// Invalide la cache TLB pour éviter des erreurs de traductions

	pt_count = UnrefPhysicalPage(pd[pd_index] & PAGE_MASK);		// On décrémente le compteur d'utilisation de l'adresse physique de la table de page

	// Si la page de table n'est plus utile
	if (pt_count == 0)		
	{
		pd[pd_index] = 0;							// On supprime l'entrée correspondante dans le répertoire de page

		if (vaddr < MIRRORING_ADDR)
			sync_pde_in_kernel_space(pd_index, pd[pd_index]);

		invlpg((vaddr_t)pt);						// On invalide la cache associé à la table de page
	}
}

/*
 * Cette fonction prend comme paramètre l'adresse VIRTUELLE d'un répertoire de page
 * (qui doit donc être mappé) et libère toute les pages étant dans l'espace d'adressage
 * de l'utilisateur
 */
void DeletePD(vaddr_t pd_vaddr)
{
	uint32_t pd_index = 0;							// Index de l'entrée du pd correspondant à l'adresse virtuelle vaddr
	uint32_t pt_index = 0;							// Index de l'entrée de la pt correspondante à l'adresse virtuelle vaddr

	pde_t *pd = (pde_t *)pd_vaddr;					// Adresse de base du répertoire de page courant
	pte_t *pt = (pte_t *)get_vm_area(1, 0);			// Adresse de base de la table de page concernée
	
	/*
	 * Comme l'espace d'adressage du noyau est commun à tous les processus on ne démmape
	 * pas cette partie là: on commence donc à partir de l'espace d'adressage utilisateur
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

				UnrefPhysicalPage(pt[pt_index] & PAGE_MASK);				// On démmape la page
				UnrefPhysicalPage(pd[pd_index] & PAGE_MASK);				// Et on décrémente le compteur d'utilisation de la table de page
			}

		PagingUnmap((vaddr_t)pt);											// On démmape la page de table mappée temporairement
	}
	FreeVmArea((vaddr_t)pt);
}

/*
 * Cette fonction retourne l'adresse physique d'une adresse virtuelle UNIQUEMENT
 * si l'adresse virtuelle est mappée en mémoire
 */
paddr_t PagingGetPhysicalAddr(vaddr_t vaddr)
{
	uint32_t pd_index = pd_offset(vaddr);			// Index de l'entrée du pd correspondant à l'adresse virtuelle vaddr
	uint32_t pt_index = pt_offset(vaddr);			// Index de l'entrée de la pt correspondante à l'adresse virtuelle vaddr
	uint32_t pg_offset = pg_offset(vaddr);			// Décalage interne dans la page

	pde_t *pd = get_current_pd_vaddr();				// Adresse de base du répertoire de page courant
	pte_t *pt = get_pt_vaddr(pd_index);				// Adresse de base de la table de page concernée

	/* Si la table de page n'est pas présente, on returne 0 (erreur) */
	if (!pde_test_flag(pd[pd_index], PDE_PRESENT))
		return 0;

	/* Si la page n'est pas présente, on returne 0 */
	if (!pte_test_flag(pt[pt_index], PTE_PRESENT))
		return 0;

	return (paddr_t) ((pt[pt_index] & PAGE_MASK) + pg_offset);		// Adresse physique de la page + décalage dans la page
} 

/*
 * Cette fonction permet de mapper une page physique à une adresse virtuelle avec les
 * droits d'accès spécifié par l'argument flags
 * Note: Si vous voulez mapper un grand intervalle d'adresses virtuelles continues et que 
 * les pages physiques associées aux adresses virtuelles importe peu, je vous conseille
 * d'utiliser PagingAutoMap basée sur cette fonction mais plus pratique
 */
void PagingMap(paddr_t paddr, vaddr_t vaddr, map_flags_t flags)
{
	uint32_t pd_index = pd_offset(vaddr);		// Index de l'entrée du pd correspondant à l'adresse virtuelle vaddr
	uint32_t pt_index = pt_offset(vaddr);		// Index de l'entrée de la pt correspondante à l'adresse virtuelle vaddr

	pde_t *pd = get_current_pd_vaddr();			// Adresse de base du répertoire de page courant
	pte_t *pt = get_pt_vaddr(pd_index);			// Adresse de base de la table de page concernée

	// On ne peut pas mapper à l'emplacement du mirroring
	if (vaddr >= MIRRORING_ADDR && vaddr < (MIRRORING_ADDR + MIRRORING_SIZE))
		return;

	// Si la table de page n'existe pas, on en alloue une
	if (!pde_test_flag(pd[pd_index], PDE_PRESENT))
	{
		paddr_t pt_allocated = GetFreePhysicalPage();

		if (!pt_allocated)								// Si pas assez de mémoire on abandonne la mapping
			return;

		pd[pd_index] = pt_allocated;					// Adresse de la table de page alloué
		pde_set_flags(pd[pd_index], PDE_PRESENT);		// Page présente en mémoire

		if (flags & VM_MAP_USER)
			pde_set_flags(pd[pd_index], PDE_USER);
		if (flags & VM_MAP_WRITE)
			pde_set_flags(pd[pd_index], PDE_WRITABLE);

		if (vaddr < MIRRORING_ADDR)
		{
			pde_clear_flags(pd[pd_index], PDE_USER);
			sync_pde_in_kernel_space(pd_index, pd[pd_index]);
		}

		invlpg((vaddr_t)pt);		// Invalide le cache par sécurité
		clear_pt(pt);				// Initialise la page de table à zéro
	}
	else if (!pte_test_flag(pt[pt_index], PTE_PRESENT))
	{	
		// Si la page n'est pas mappé alors on incrémente le conteur d'utilisation de la page de table
		// (Situation normal de mapping)
		RefPhysicalPage(pd[pd_index] & PAGE_MASK);
	}
	else
	{
		// Erreur, page déjà mappé, par sécurité on fait crasher le noyau
		// Même si on pourrait automatiquement démmaper l'ancienne ressource
		// mais je préfère tout faire crasher pour éviter corruption des données
		panic("Tentative de mapping à une adresse virtuelle déjà utilisée: 0x%08x", vaddr);
	}

	pt[pt_index] = paddr;									// Adresse de la table de page alloué
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
 * NOTE: Toutes les entrées du répertoire de pages sont effacées, donc faites
 * attention !!
 */
void PagingCopyKernelSpace(vaddr_t dst_pd, vaddr_t src_pd)
{
	vaddr_t *dst_pd_ptr = (vaddr_t *)dst_pd;
	vaddr_t *src_pd_ptr = (vaddr_t *)src_pd;
	clear_pd(dst_pd_ptr);

	paddr_t dst_paddr_of_pd = PagingGetPhysicalAddr(dst_pd);

	// Copie des pages communes à tous les processus (le noyau)
	for (unsigned int pd_index = 0; pd_index < pd_offset(MIRRORING_ADDR); pd_index++)
		dst_pd_ptr[pd_index] = src_pd_ptr[pd_index];
	
	// Mise en place du mirroring (différent pour chaque contexte mémoire)
	dst_pd_ptr[pd_offset(MIRRORING_ADDR)] = dst_paddr_of_pd & PAGE_MASK;		// Pour pouvoir modifer le pd ultérieurement
	dst_pd_ptr[pd_offset(MIRRORING_ADDR)] |= PDE_PRESENT | PDE_WRITABLE;		// Option du mirroring
}

/*
 * Cette fonction est semblable à PagingMap sauf que les pages physiques sont prise au hasard
 * pour la mapping et que cette fonction s'avvère très utile pour mapper un intervalle d'adresses
 * virtuelles simplement
 */
void PagingAutoMap(vaddr_t vaddr_start, count_t nb_pages, map_flags_t flags)
{
	paddr_t tmp_paddr;
	
	for (unsigned int i = 0; i < nb_pages; i++)
	{
		tmp_paddr = GetFreePhysicalPage();
		if (!tmp_paddr)
			panic("PagingAutoMap(): Pas assez de mémoire physique");

		PagingMap(tmp_paddr, vaddr_start + (i * PAGE_SIZE), flags);
		UnrefPhysicalPage(tmp_paddr);
	}
}