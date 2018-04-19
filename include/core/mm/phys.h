#pragma once
#include <types.h>

#define PAGE_SIZE		4096
#define PAGE_MASK		(~(PAGE_SIZE - 1))
#define PAGE_SHIFT		12

#define PHYS_ALIGN_INF(x)	(x & PAGE_MASK)					// Aligne un adresse à la page du dessous (0x1234 devient 0x1000)
#define PHYS_ALIGN_SUP(x)	((x & PAGE_MASK) + PAGE_SIZE)	// Aligne un adresse à la page du supérieur (0x1234 devient 0x2000)
#define ADDR_TO_PAGE(x)		(x >> PAGE_SHIFT)				// Convertion d'une adresse en mémoire au numéro de page correspondant

#define PHYS_PAGE_RESERVED	0xFFFFFFFF

typedef unsigned int page_t;
typedef unsigned int paddr_t;

struct PhysicalPage
{
	count_t count;							// Nombre de fois que la page est référencée (0xFFFFFFFF = impossible de déréférencer)
	//struct PhysicalPage *prev, *next;		// Liste chainée
};

void print_mem_stat(void);
paddr_t GetEndOfMemMap(void);
uint32_t get_free_page_num(void);
uint32_t get_used_page_num(void);
paddr_t GetFreePhysicalPage(void);
uint32_t get_nb_physical_pages(void);
int RefPhysicalPage(const paddr_t paddr);
int UnrefPhysicalPage(const paddr_t paddr);
void SetPhysPageReserved(const page_t page);
struct PhysicalPage *AddrToPageDescriptor(const paddr_t addr);
void PhysMemSetup(const unsigned int mem_avaible, const unsigned int end_of_kernel);
