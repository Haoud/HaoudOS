#include <assert.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/mm/phys.h>
#include <driver/bochs/bochs.h>

static struct PhysicalPage *mem_map = NULL;					// Listes des pages présentes en mémoire

static size_t mem_map_size = 0;								// Nombre de page physique en mémoire (mem_map_size - 1 = index de la derniere page dans mem_map)
static count_t free_page_nr = 0;							// Nombre de page libres
static count_t used_page_nr = 0;							// Nombre de page utilisés ou reservés

/*
 * Cette fonction initialise le gestionnaire de page physique: elle reçoit en
 * argument la taille de la mémoire en kilo-octet ainsi que la taille du noyau
 * aussi en kilo-octet et se charge d'initialiser l'allocation de page physique
 * mais aussi de définir les pages réservés (comme celle du BIOS ou du noyau)
 *
 * TODO: Dans le chargeur d'HaoudOS passer une carte mémoire au noyau
 */
void PhysMemSetup(const unsigned int mem_avaible, const unsigned int end_of_kernel)
{
 	BochsPrintf("\n[INFO] Preparing physical memory management...\n");

	mem_map = (struct PhysicalPage *) (PHYS_ALIGN_SUP(end_of_kernel));				// Liste de page juste après le noyau
	mem_map_size = (ADDR_TO_PAGE(mem_avaible * 1024));								// Numéro de la dernière page disponible

	for (unsigned int i = 0; i < mem_map_size; i++)
		mem_map[i].count = 0;

	BochsPrintf("[DEBUG] Addr of mem_map: 0x%p\n", mem_map);
	BochsPrintf("[DEBUG] Number to object in mem_map: %u\n", mem_map_size);
	BochsPrintf("[DEBUG] End of mem_map: 0x%p\n", &mem_map[mem_map_size - 1]);

	free_page_nr = mem_map_size;						// Nombre de page physique de libre (toute pour l'instant)

	/* Reservé pour la pile du noyau et le BIOS */
	for (unsigned int i = ADDR_TO_PAGE(0x0);
		     i < ADDR_TO_PAGE(0x10000); 
			 i++)
		SetPhysPageReserved(i);

	/* Reservé pour le BIOS, la mémoire vidéo et la carte mère */
	for (unsigned int i = ADDR_TO_PAGE(0xA0000);
		     i < ADDR_TO_PAGE(0x100000); 
			 i++)
		SetPhysPageReserved(i);

	/* Reservé pour le noyau */
	for (unsigned int i = ADDR_TO_PAGE(0x100000);
		     i < ADDR_TO_PAGE(PHYS_ALIGN_SUP(end_of_kernel)); 
			 i++)
		SetPhysPageReserved(i);

	/* Reservé pour le tableau de descriptions des pages physique */
	for (unsigned int i = ADDR_TO_PAGE(PHYS_ALIGN_SUP(end_of_kernel));
		     i < ADDR_TO_PAGE(PHYS_ALIGN_SUP((end_of_kernel + (mem_map_size * sizeof(struct PhysicalPage))))) ; 
			 i++)
		SetPhysPageReserved(i);

	BochsPrintf("[INFO] Actually free physical pages: %u\n", free_page_nr);
	BochsPrintf("[INFO] Actually used physical page: %u\n", used_page_nr);

	BochsPrintf("[INFO] Actually memory used: %u %%\n", (used_page_nr * 100 / free_page_nr));		// Pourcentage de mémoire utilisé (tronqué: 0.99% = 0%, 100% = pas bon signe)
}

void print_mem_stat(void)
{
	debugk("\n==========[ Statistique d'utilisation de la mémoire physique ]==========\n");
	debugk("       Mémoire physique disponible: %uMo + %uko (%uo) = %u%%\n", ((free_page_nr * PAGE_SIZE) / 1024) / 1024, ((free_page_nr * PAGE_SIZE) / 1024) % 1024, free_page_nr * PAGE_SIZE, 100 - (used_page_nr * 100 / free_page_nr));
	debugk("        Mémoire physique utilisé: %uMo + %uko (%uo) = %u%%\n", ((used_page_nr * PAGE_SIZE) / 1024) / 1024, ((used_page_nr * PAGE_SIZE) / 1024) % 1024, used_page_nr * PAGE_SIZE, (used_page_nr * 100 / free_page_nr));
	debugk("     Mémoire utilisée par la liste des pages physique: %uko (%uo)\n", (mem_map_size * sizeof(struct PhysicalPage) / 1024), mem_map_size * sizeof(struct PhysicalPage));
	debugk("========================================================================\n\n");
}

/*
 * Cette fonction permet de définir une page (représentée par son numéro à partir de
 * l'adresse 0) comme étant réservée: Elle ne peut être alloué ni désalloué: c'est 
 * très utile notamment pour le code du noyau et les périphériques (comme le BIOS
 * ou la mémoire vidéo mappée en mémoire)
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

	/* Si la page est utilisé on affiche une message de debug pour prévenir.. */
	if (mem_map[page].count != 0)
		debugk("Warning: Trying to reserved a used page(%08x)...\n", page * 4096);

	mem_map[page].count = PHYS_PAGE_RESERVED;	// On définit la page comme étant réservé

	free_page_nr--;		
	used_page_nr++;		
}

/*
 * Cette fonction permet d'obtenir un cadre de page physique
 * libre en mémoire physique (compteur de référence à 0)
 * TODO: Optimiser cette fonction avec une liste chainée
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
 * Permet de référencer un page physique non libre, c'est à dire que la page
 * est utilisé plusieurs fois. Par exemple, si le compteur de référence est 
 * à 2, alors il faut 2 appels à la fonction UnrefPhysicalPage pour libérer
 * réellement la page physique
 */
int RefPhysicalPage(const paddr_t paddr)
{
	struct PhysicalPage *pdesc = NULL;
	pdesc = AddrToPageDescriptor(paddr);

	if (!paddr)
		return PHYS_PAGE_RESERVED;

	if (pdesc->count == PHYS_PAGE_RESERVED)
		panic("Tentavive de référencement d'une page physique réservé");

	return ++pdesc->count;
}

/*
 * Cette fonction permet de décrémenter le compteur d'utilisation
 * d'une page physique et éventuellement la libérer si le compteur
 * devient nul
 */
int UnrefPhysicalPage(const paddr_t paddr)
{
	struct PhysicalPage *pdesc = NULL;
	pdesc = AddrToPageDescriptor(paddr);
	
	if (!paddr)
		return PHYS_PAGE_RESERVED;

	if (pdesc->count == PHYS_PAGE_RESERVED)
		panic("Tentavive de libération d'une page physique réservé");

	if (pdesc->count == 0)
		panic("Tentative de libération d'une page physique déjà libre");


	pdesc->count--;
	if (pdesc->count > 0)		// Si la page est toujours utilisé on on n'actualise pas les statistique d'utilisation
		goto out;
		
	used_page_nr--;
	free_page_nr++;
	
out:
	return pdesc->count;
}

/*
 * Cette fonction revoit le descripteur de page physique associé
 * à une adresse physique addr
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
 * Cette fonction retourne l'adresse physique situé l'octet d'après le dernier descripteur
 * de page physique: utilisé notamment par la Pagination
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