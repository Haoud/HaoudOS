#include <assert.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/mm/phys.h>
#include <driver/bochs/bochs.h>

static struct PhysicalPage *mem_map = NULL;					// Listes des pages pr�sentes en m�moire

static size_t mem_map_size = 0;								// Nombre de page physique en m�moire (mem_map_size - 1 = index de la derniere page dans mem_map)
static count_t free_page_nr = 0;							// Nombre de page libres
static count_t used_page_nr = 0;							// Nombre de page utilis�s ou reserv�s

/*
 * Cette fonction initialise le gestionnaire de page physique: elle re�oit en
 * argument la taille de la m�moire en kilo-octet ainsi que la taille du noyau
 * aussi en kilo-octet et se charge d'initialiser l'allocation de page physique
 * mais aussi de d�finir les pages r�serv�s (comme celle du BIOS ou du noyau)
 *
 * TODO: Dans le chargeur d'HaoudOS passer une carte m�moire au noyau
 */
void PhysMemSetup(const unsigned int mem_avaible, const unsigned int end_of_kernel)
{
 	BochsPrintf("\n[INFO] Preparing physical memory management...\n");

	mem_map = (struct PhysicalPage *) (PHYS_ALIGN_SUP(end_of_kernel));				// Liste de page juste apr�s le noyau
	mem_map_size = (ADDR_TO_PAGE(mem_avaible * 1024));								// Num�ro de la derni�re page disponible

	for (unsigned int i = 0; i < mem_map_size; i++)
		mem_map[i].count = 0;

	BochsPrintf("[DEBUG] Addr of mem_map: 0x%p\n", mem_map);
	BochsPrintf("[DEBUG] Number to object in mem_map: %u\n", mem_map_size);
	BochsPrintf("[DEBUG] End of mem_map: 0x%p\n", &mem_map[mem_map_size - 1]);

	free_page_nr = mem_map_size;						// Nombre de page physique de libre (toute pour l'instant)

	/* Reserv� pour la pile du noyau et le BIOS */
	for (unsigned int i = ADDR_TO_PAGE(0x0);
		     i < ADDR_TO_PAGE(0x10000); 
			 i++)
		SetPhysPageReserved(i);

	/* Reserv� pour le BIOS, la m�moire vid�o et la carte m�re */
	for (unsigned int i = ADDR_TO_PAGE(0xA0000);
		     i < ADDR_TO_PAGE(0x100000); 
			 i++)
		SetPhysPageReserved(i);

	/* Reserv� pour le noyau */
	for (unsigned int i = ADDR_TO_PAGE(0x100000);
		     i < ADDR_TO_PAGE(PHYS_ALIGN_SUP(end_of_kernel)); 
			 i++)
		SetPhysPageReserved(i);

	/* Reserv� pour le tableau de descriptions des pages physique */
	for (unsigned int i = ADDR_TO_PAGE(PHYS_ALIGN_SUP(end_of_kernel));
		     i < ADDR_TO_PAGE(PHYS_ALIGN_SUP((end_of_kernel + (mem_map_size * sizeof(struct PhysicalPage))))) ; 
			 i++)
		SetPhysPageReserved(i);

	BochsPrintf("[INFO] Actually free physical pages: %u\n", free_page_nr);
	BochsPrintf("[INFO] Actually used physical page: %u\n", used_page_nr);

	BochsPrintf("[INFO] Actually memory used: %u %%\n", (used_page_nr * 100 / free_page_nr));		// Pourcentage de m�moire utilis� (tronqu�: 0.99% = 0%, 100% = pas bon signe)
}

void print_mem_stat(void)
{
	debugk("\n==========[ Statistique d'utilisation de la m�moire physique ]==========\n");
	debugk("       M�moire physique disponible: %uMo + %uko (%uo) = %u%%\n", ((free_page_nr * PAGE_SIZE) / 1024) / 1024, ((free_page_nr * PAGE_SIZE) / 1024) % 1024, free_page_nr * PAGE_SIZE, 100 - (used_page_nr * 100 / free_page_nr));
	debugk("        M�moire physique utilis�: %uMo + %uko (%uo) = %u%%\n", ((used_page_nr * PAGE_SIZE) / 1024) / 1024, ((used_page_nr * PAGE_SIZE) / 1024) % 1024, used_page_nr * PAGE_SIZE, (used_page_nr * 100 / free_page_nr));
	debugk("     M�moire utilis�e par la liste des pages physique: %uko (%uo)\n", (mem_map_size * sizeof(struct PhysicalPage) / 1024), mem_map_size * sizeof(struct PhysicalPage));
	debugk("========================================================================\n\n");
}

/*
 * Cette fonction permet de d�finir une page (repr�sent�e par son num�ro � partir de
 * l'adresse 0) comme �tant r�serv�e: Elle ne peut �tre allou� ni d�sallou�: c'est 
 * tr�s utile notamment pour le code du noyau et les p�riph�riques (comme le BIOS
 * ou la m�moire vid�o mapp�e en m�moire)
 */
void SetPhysPageReserved(const page_t page)
{
	if (page > (mem_map_size - 1))			// Si la page est en dehors de la RAM disponible
		return;								// On quitte

	if (mem_map[page].count == PHYS_PAGE_RESERVED)
	{
		debugk("SetPhysPageReserved(): Page #%u already reserved\n", page);
		return;
	}

	/* Si la page est utilis� on affiche une message de debug pour pr�venir.. */
	if (mem_map[page].count != 0)
		debugk("Warning: Trying to reserved a used page(%08x)...\n", page * 4096);

	mem_map[page].count = PHYS_PAGE_RESERVED;	// On d�finit la page comme �tant r�serv�

	free_page_nr--;		
	used_page_nr++;		
}

/*
 * Cette fonction permet d'obtenir un cadre de page physique
 * libre en m�moire physique (compteur de r�f�rence � 0)
 * TODO: Optimiser cette fonction avec une liste chain�e
 */
paddr_t GetFreePhysicalPage(void)
{
	unsigned int i = 0;

	for (i = 0; i < (mem_map_size - 1); i++)
	{
		if (mem_map[i].count == 0)
			goto free_phys_page_found;
	}

	debugk("GetFreePhysicalPage(): No enought memory !\n");
	return 0;		// Pas de page physique de disponilbe

free_phys_page_found:
	used_page_nr++;
	free_page_nr--;

	mem_map[i].count = 1;
	return (i << PAGE_SHIFT);
}

/*
 * Permet de r�f�rencer un page physique non libre, c'est � dire que la page
 * est utilis� plusieurs fois. Par exemple, si le compteur de r�f�rence est 
 * � 2, alors il faut 2 appels � la fonction UnrefPhysicalPage pour lib�rer
 * r�ellement la page physique
 */
int RefPhysicalPage(const paddr_t paddr)
{
	struct PhysicalPage *pdesc = NULL;
	pdesc = AddrToPageDescriptor(paddr);

	if (!paddr)
		return PHYS_PAGE_RESERVED;

	if (pdesc->count == PHYS_PAGE_RESERVED)
		panic("Tentavive de r�f�rencement d'une page physique r�serv�");

	return ++pdesc->count;
}

/*
 * Cette fonction permet de d�cr�menter le compteur d'utilisation
 * d'une page physique et �ventuellement la lib�rer si le compteur
 * devient nul
 */
int UnrefPhysicalPage(const paddr_t paddr)
{
	struct PhysicalPage *pdesc = NULL;
	pdesc = AddrToPageDescriptor(paddr);
	
	if (!paddr)
		return PHYS_PAGE_RESERVED;

	if (pdesc->count == PHYS_PAGE_RESERVED)
		panic("Tentavive de lib�ration d'une page physique r�serv�");

	if (pdesc->count == 0)
		panic("Tentative de lib�ration d'une page physique d�j� libre");


	pdesc->count--;
	if (pdesc->count > 0)		// Si la page est toujours utilis� on on n'actualise pas les statistique d'utilisation
		goto out;
		
	used_page_nr--;
	free_page_nr++;
	
out:
	return pdesc->count;
}

/*
 * Cette fonction revoit le descripteur de page physique associ�
 * � une adresse physique addr
 */
struct PhysicalPage *AddrToPageDescriptor(const paddr_t addr)
{
	page_t page_nr = ADDR_TO_PAGE(addr);

	if (page_nr > (mem_map_size - 1))
		return NULL;
	else
		return &mem_map[page_nr];
}

/*
 * Cette fonction retourne l'adresse physique situ� l'octet d'apr�s le dernier descripteur
 * de page physique: utilis� notamment par la Pagination
 */
paddr_t GetEndOfMemMap(void)
{
	return (paddr_t) (((uint32_t)&mem_map[mem_map_size - 1]) + sizeof(struct PhysicalPage));
}

uint32_t get_free_page_num(void)
{
	return free_page_nr;
}

uint32_t get_used_page_num(void)
{
	return used_page_nr;
}

uint32_t get_nb_physical_pages(void)
{
	return used_page_nr + free_page_nr;
}