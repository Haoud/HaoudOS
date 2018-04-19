/*
 * This file was created on Wed Mar 28 2018
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
#pragma once

#include <types.h>
#include <core/mm/phys.h>

#define CR0_PAGING_BIT				0x80000000			// Bit d'activation de la pagination
#define CR0_WRITE_PROTECT_BIT		0x00010000			// Même en mode superviseur on respecte les pages marquées en lecture seule
#define CR4_PSE_BIT					0x00000010			// Pour activer les pages de 4MO (Pentium et plus seulement)

#define PD_NR_ENTRIES		1024			// Nombre d'entrée dans un répertoire de page
#define PD_SIZE				4096			// Taille d'un répertoire de page

#define PT_NR_ENTRIES		1024			// Nombre d'entrée dans une table de page
#define PT_SIZE				4096			// Taille d'un table de page

#define MIRRORING_SIZE		0x400000		// Taille réservée au mirroring (4MO)
#define MIRRORING_ADDR		0x3FC00000		// Adresse virtuelle où le mirroring sera installé

#define pd_offset(vaddr)		(((vaddr) & 0xFFC00000) >> 22)				// Cette macro retourne l'index de l'adresse virtuelle correspondant à l'entrée du répertoire de table
#define pt_offset(vaddr)		(((vaddr) & 0x003FF000) >> 12)				// Cette macro retourne l'index de l'adresse virtuelle correspondant à la page de table
#define pg_offset(vaddr)		((vaddr) & 0x00000FFF)						// Cette macro retourne l'offset de l'adresse virtuelle dans une page

#define clear_pd(pd)	memset((void *)pd, 0, PD_SIZE);		// Initialisation à zéro du répertoire de pages
#define clear_pt(pt)	memset((void *)pt, 0, PT_SIZE);		// Initialisation à zéro de la table de pages

typedef uint32_t vaddr_t;
typedef uint32_t pde_t;
typedef uint32_t pte_t;

#define PDE_PRESENT				0x00000001	
#define PDE_WRITABLE			0x00000002		
#define PDE_USER				0x00000004	
#define PDE_WRITE_THOUGH		0x00000008		
#define PDE_NOT_CACHEABLE		0x00000010
#define PDE_ACCESSED			0x00000020	
#define PDE_PAGE_4MO			0x00000080

#define PTE_PRESENT				0x00000001	
#define PTE_WRITABLE			0x00000002		
#define PTE_USER				0x00000004	
#define PTE_WRITE_THOUGH		0x00000008		
#define PTE_NOT_CACHEABLE		0x00000010
#define PTE_ACCESSED			0x00000020	
#define PTE_DIRTY				0x00000040

#define VM_MAP_USER				0x00000001
#define VM_MAP_WRITE			0x00000002
#define VM_MAP_ATOMIC			0x00000004

typedef flags_t map_flags_t;

#define pde_set_flags(pde, flags)		__paging_set_bits(pde, flags)			// Positionne un flags sur la pde
#define pde_clear_flags(pde, flags)		__paging_clear_bits(pde, flags)			// Enlève un flags de la pde
#define pde_test_flag(pde, flag)		(pde & (flag))							// Teste si le flags est présente dans le pde

#define pte_set_flags(pte, flags)		__paging_set_bits(pte, flags)			// Positionne un flags sur la pte
#define pte_clear_flags(pte, flags)		__paging_clear_bits(pte, flags)			// Enlève un flags de la pte
#define pte_test_flag(pte, flag)		(pte & (flag))							// Teste si le flags est présente dans le pte
		
#define __paging_set_bits(var, mask)	(var |= (mask))
#define __paging_clear_bits(var, mask)	(var &= ~(mask))

#define get_current_pd_vaddr() (void *) (MIRRORING_ADDR + PAGE_SIZE * pd_offset(MIRRORING_ADDR))
#define get_pt_vaddr(pd_index) (void *) (MIRRORING_ADDR + (PAGE_SIZE * pd_index))

#define get_current_pd_paddr()		GetCurrentPD()

/*
struct pde_t
{
	uint8_t present : 1;			// 1 = Page présente et mappée
	uint8_t read_write : 1;			// 1 = page accesible en lecture/écriture, sinon en lecture seule
	uint8_t user : 1;				// 1 = Page accesible à tous, sinon seulement au noyau
	uint8_t write_through : 1;		// 1 = write through, sinon write back
	uint8_t cache_disable : 1;		// 1 = cache désactivé, sinon le cache est activé
	uint8_t accessed : 1;			// 1 = page accédée, sinon non
	uint8_t unused : 1;				// Doit être 0
	uint8_t page_size : 1;			// Taille des pages (1 = 4MO, 0 = 4ko)
	uint8_t ignored : 1;			// Ignoré
	uint8_t avaible : 3;			// Libre pour le système d'exploitation

	uint32_t pt_base : 20;			// Adresse de base du répertoire de page aligné sur 4ko
};

struct pte_t
{
	uint8_t present : 1;			// 1 = Page présente et mappée
	uint8_t read_write : 1;			// 1 = page accesible en lecture/écriture, sinon en lecture seule
	uint8_t user : 1;				// 1 = Page accesible à tous, sinon seulement au noyau
	uint8_t write_through : 1;		// 1 = write through, sinon write back
	uint8_t cache_disable : 1;		// 1 = cache désactivé, sinon le cache est activé
	uint8_t accessed : 1;			// 1 = page accédée, sinon non
	uint8_t dirty : 1;				// Doit être 0
	uint8_t unused : 1;				// Ne sert à rien pour les PTE
	uint8_t global : 1;				// Gestion du cache
	uint8_t avaible : 3;			// Libre pour le système d'exploitation

	uint32_t pg_base : 20;			// Adresse de base du répertoire de page aligné sur 4ko
};

   Ces structures sont correct mais ne fonctionne pas avec le compilateur (problèmes d'alignement) :(
*/

void Flush_TLB(void);
void SetupPaging(void);
void EnablePaging(void);
void DisablePaging(void);
void invlpg(vaddr_t addr);
paddr_t GetCurrentPD(void);
void PagingUnmap(vaddr_t vaddr);
void DeletePD(vaddr_t pd_vaddr);
void SetCurrentPD(vaddr_t pd_addr);
paddr_t PagingGetPhysicalAddr(vaddr_t vaddr);
void PagingCopyKernelSpace(vaddr_t dst_pd, vaddr_t src_pd);
void PagingMapHelper(pde_t *pd, paddr_t paddr, vaddr_t vaddr);
void PagingMap(paddr_t paddr, vaddr_t vaddr, map_flags_t flags);
void PagingAutoMap(vaddr_t vaddr_start, count_t nb_pages, map_flags_t flags);




