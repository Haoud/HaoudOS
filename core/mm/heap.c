/*
 * This file was created on Fri Apr 13 2018
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
#include <const.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <core/mm/heap.h>
#include <driver/bochs/bochs.h>

static struct heap_chunk_header *first_chunk;	// On ne peut pas utilise kmalloc

// Variables utilisées pour briser la récursivité entre vm_area et le heap
static count_t nb_chunks;						// Nombre total de chunks
static count_t nb_chunks_used;					// Nombre total de chunks utilisés
static count_t nb_chunks_free;					// Nombre total de chunks libre

static uint32_t heap_memory_allocated;			// Taille total alloué pour le heap
static uint32_t heap_free_memory;				// Taille actuellement disponible(sans compter les en-têtes)

/*
 * Cete fonction initialise le heap: au départ, le heap comporte seulemnent un gros bloc de
 * 16ko. Par la suite, ce bloc sera scindé en plusieurs partie et la taille du heap augmentée
 * si besoin (de 16 en 16 ko)
 */
void SetupHeap(vaddr_t start)
{
	struct heap_chunk_header *chunk;

	// On met en place la mémoire du heap (sinon l'écriture du header provoquera une faute de page)
	AllocMemForHeap(start);

	chunk = (struct heap_chunk_header *)start;				// Début de l'en tête = début du block initial de 16ko pour le heap

	chunk->magic = HEAP_HEADER_MAGIC;							// Pour vérifie la validité du chunk
	chunk->size = HEAP_INITIAL_SIZE - HEAP_HEADER_SIZE;		// Garde toujours de la place pour l'en tête du chunk suivant si ce chunk est plein
	chunk->flags = 0;											// Pas d'options pour l'instant (et le chunk n'est pas utilisé)

	/* Il n'y a qu'un seul block pour l'instant*/
	list_singleton(first_chunk, chunk);

	heap_memory_allocated = HEAP_INITIAL_SIZE;
	heap_free_memory = chunk->size;

	nb_chunks = 1;
	nb_chunks_free = 1;
	nb_chunks_used = 0;

	/*
	 * NOTE: Le heap garde toujours de la place pour l'en tête du chunk afin d'éviter
	 * une récursivité infinie avec kmalloc
	 *
	 * La base de l'en tête détermine le début de l'espace virtuel réservé pour le bloc: 2 blocs 
	 * ne sont donc pas forcément côte à côte en mémoire s'il sont côte à côte dans la liste chainée.
	 *
	 * Cette particularité est intérésente car le heap peut fonctionner même en cas d'utilisation
	 * excessive de l'espace virtuel du noyau: s'il reste de la place, on peut l'utiliser pour le
	 * heap: si le heap aurait été continu: il aurait été probablement impossible de satisfaire la
	 * requete d'allocation.
	 * Certes, une très faible partie de la mémoire partie est perdu:
	 *		- Pour stocker l'en tête du bloc (24 octets par bloc)
	 *		- Et en cas de fragmentation interne importante
	 */

	BochsPrintf("\n[DEBUG] Adresse of first chunk: 0x%p\n", chunk);
	BochsPrintf("[DEBUG] Size of first chunk: %u o\n", chunk->size);
	BochsPrintf("[DEBUG] Size of chunk header: %u o\n", HEAP_HEADER_SIZE);

	BochsPrintf("[INFO] Safe unallocated size for heap: %u o\n", HEAP_SAFE_MIN_MEM);

	BochsPrintf("[INFO] Kernel heap initialized\n");
}

/*
 * Cette fonction permet d'étendre le heap de n pages grâce à vm_area
 *
 * NOTE: il doit avoir assez de place de le heap pour allouer 2 descripteurs:
 *	  - Le descripteur de vm_area;
 *	  - Et le descripteur de l'en tête du chunk;
 *
 * Soit environ 48 octets. Si cette condition n'est pas remplit, on ne peut
 * pas étendre le heap et il se produit un interblocage avec vm_area qui 
 * obligera à un reboot de la machine ou à une erreur.
 *
 * Normalement, kmalloc gère cet interblocage mais pas kmalloc_no_security
 * pour re-éviter un interblocage du à la limitation en cas d'espace du heap
 * faible (impossible d'allouer un onjet lorsque il reste moins de 90 octets
 * continu), mais cette vérification n'est pas faite sous kmalloc_no_security
 * car vm_area à besoin de cet espace pour étendre le heap
 *
 * Oui je sais, c'est compliqué et mes explications ne sont pas très claires
 */
bool_t ExtendHeap(count_t nb_pages)
{
	vaddr_t alloc_vaddr;
	paddr_t phys_page;
	count_t num_pages_requied = nb_pages;
	struct heap_chunk_header *new_chunk = NULL;

	alloc_vaddr = get_vm_area(nb_pages, 0);		// Requète de 4 pages virtuelles
	new_chunk = (struct heap_chunk_header *) (alloc_vaddr);

	if (!alloc_vaddr)
	{
		debugk("Unable to expand kernel heap\n");
		return FALSE;
	}

	while (num_pages_requied--)
	{
		phys_page = GetFreePhysicalPage();
		if (!phys_page)
		{
			debugk("Unable to expand kernel heap: No enought physical memory\n");
			//DebugHeap();		
			FreeVmArea(alloc_vaddr);
			return FALSE;
		}

		PagingMap(phys_page, alloc_vaddr, VM_MAP_WRITE);
		UnrefPhysicalPage(phys_page);

		alloc_vaddr += PAGE_SIZE;
	}
	new_chunk->magic = HEAP_HEADER_MAGIC;
	new_chunk->flags = 0;
	new_chunk->size = (nb_pages * PAGE_SIZE) - HEAP_HEADER_SIZE;

	// On actualise les champs anti-récursivité infinie
	heap_memory_allocated += (nb_pages * PAGE_SIZE);

	heap_free_memory += new_chunk->size;
	nb_chunks++;
	nb_chunks_free++;

	list_add_before(first_chunk, new_chunk);			// Ajoute le chunk au début de la liste pour + de performances
	return TRUE;
}

/*
 * Cette fonction alloue la mémoire (16ko) pour que le heap fonctionne correctement au départ sans
 * provoquer de faute de page: par la suite le heap peut être étendu grâce à ExtendHeap
 */
void AllocMemForHeap(vaddr_t start)
{
	paddr_t phys_page = 0;
	vaddr_t virt_addr = start;
	uint32_t heap_size = HEAP_INITIAL_SIZE;

	while (heap_size)
	{
		phys_page = GetFreePhysicalPage();		// Alloue une nouvelle page physique
		PagingMap(phys_page, virt_addr, VM_MAP_ATOMIC | VM_MAP_WRITE);			// Mapper la page physique (et incrémente son compeur d'utilisation) à l'adresse virtuelle  
		UnrefPhysicalPage(phys_page);			// Décrémente le compteur d'utilisation de cette page

		virt_addr += PAGE_SIZE;
		heap_size -= PAGE_SIZE;
	}
}

/*
 * Cette fonction alloue un bloc de n octets et le retourne à l'appelant ou NULL si une erreur
 * quelconque suvient (pas assez de mémoire...)
 *
 * TODO: Utiliser des sémaphores pour éviter des interblocages entre processus
 */
void *kmalloc(size_t size)
{
	struct heap_chunk_header *current_chunk_header;
	struct heap_chunk_header *split_chunk;
	int nb;
	
	// Si le heap n'est pas initialisé
	if (!first_chunk)
		panic("Le heap n'a pas été correctement initialisé");

	// On alloue au moins 8 octets pour éviter une fragmentation interne trop grande
	if (size < HEAP_MINIMAL_SIZE)
		size = HEAP_MINIMAL_SIZE;

start_chunk_finding:
	
	list_foreach(first_chunk, current_chunk_header, nb)
	{
		// Si le bloc est utilisé, on passe ce bloc
		if (current_chunk_header->flags & CHUNK_USED)
			continue;

		// Si l'en tête est corrompu par le bloc précédent (overflow)
		if (current_chunk_header->magic != HEAP_HEADER_MAGIC)
			panic("Le tas du noyau est corrompu !!");

		if ((heap_free_memory / nb_chunks_free) < HEAP_SAFE_MIN_MEM ||
			heap_free_memory - (size + HEAP_HEADER_SIZE) < HEAP_SAFE_MIN_MEM)
		{
			if (ExtendHeap(4))
				goto start_chunk_finding;
			else
				return NULL;
		}

		// Ici, on a trouver un chunk libre, on vérifie sa taille (taille demandé + place pour l'en tête du chunk)
		if(current_chunk_header->size == size)
		{
			set_bits(current_chunk_header->flags, CHUNK_USED);																// On va retourner ce bloc, il donc maintenant utilisé 
			
			nb_chunks++;																									// Un nouveau chunk a été crée
			nb_chunks_free--;
			nb_chunks_used++;																								// Un chunk existant est désormais utilisé
			heap_free_memory -= current_chunk_header->size;
			
			return (void *)(((uint32_t)current_chunk_header) + HEAP_HEADER_SIZE);											// Adresse de base du block (après l'en tête du block)
		}
		if (current_chunk_header->size >= (size + HEAP_HEADER_SIZE))
		{
			// Ici on coupe le bloc en deux partie
			split_chunk = (struct heap_chunk_header *) ((uint32_t) current_chunk_header + HEAP_HEADER_SIZE + size);			// Espace réservé pour l'en tête du chunk
			split_chunk->magic = HEAP_HEADER_MAGIC;																			// Pour détécter les overflows
			split_chunk->size = current_chunk_header->size - size - HEAP_HEADER_SIZE;										// La taille du nouveau bloc
			list_add_after(current_chunk_header, split_chunk);																// On ajoute l'en tête après le block
			clear_bits(split_chunk->flags, CHUNK_USED);																		// Le nouveau bloc crée est disponible

			current_chunk_header->size = size;																				// On actualise la taille du bloc coupé en deux
			set_bits(current_chunk_header->flags, CHUNK_USED);																// On va retourner ce bloc, il donc maintenant utilisé 

			nb_chunks++;			// Un nouveau chunk a été crée
			nb_chunks_used++;		// Un chunk existant est désormais utilisé
			// Un chunk a été créer (et est libre) et un autre a été utilisé, donc pas besoin de toucher à nb_chunks_free
			heap_free_memory -= (current_chunk_header->size + HEAP_HEADER_SIZE);

			return (void *) (((uint32_t) current_chunk_header) + HEAP_HEADER_SIZE);											// Adresse de base du block (après l'en tête du block)
		}

	}

	// Si pas assez de place on étend le heap
	ExtendHeap((size / PAGE_SIZE) + 2);			// Taille max des objets dans le heap: environ 16 ko
	goto start_chunk_finding;					// Normalement on DOIT trouver un objet
}

/*
 * Cette fonction affiche des informations sur le heap dans le port 0xe9 de Bochs
 * à des fins de déboguage
 */
void DebugHeap(void)
{
	struct heap_chunk_header *current_chunk;
	int nb_element = 0;

	debugk("\nDebugging kernel heap...\n\n");

	debugk("Memory allocated for kernel heap: %u \no", heap_memory_allocated);
	debugk("Free memory in kernel heap: %u o\n", heap_free_memory);
	debugk("Number of chunks: %u\n", nb_chunks);
	debugk("Number of used chunks: %u\n", nb_chunks_free);
	debugk("Number of free chunks: %u\n\n", nb_chunks_used);

	debugk("Dumping list of chunks...\n\n");

	list_foreach(first_chunk, current_chunk, nb_element)
	{
		debugk("Base of chunk: 0x%p, Size of chunk: %uo\n", current_chunk, current_chunk->size);
		debugk("Next Chunk at 0x%p, Previous chunk at 0x%p\n", current_chunk->next, current_chunk->prev);
		debugk("Magic number of chunk: 0x%0X\n", current_chunk->magic);

		debugk("\n");
	}
}

/*
 * Cette fonction est semblable à kmalloc MAIS ne fait AUCUNE vérification de sécurité (contre
 * les interblocages, les overflows...), elle est donc plus rapide mais plus dangeureuse.
 * En théorie, seul vm_area à le droit d'utiliser cette fonction, pour éviter un interblocage
 * a cause d'une sécurité contre les interblocages :)
 */
void *kmalloc_no_security(size_t size)
{
	struct heap_chunk_header *current_chunk_header;
	struct heap_chunk_header *split_chunk;
	int nb;

	// On alloue au moins 8 octets pour éviter une fragmentation interne trop grande
	if (size < HEAP_MINIMAL_SIZE)
		size = HEAP_MINIMAL_SIZE;

	list_foreach(first_chunk, current_chunk_header, nb)
	{
		// Si le bloc est utilisé, on passe ce bloc
		if (current_chunk_header->flags & CHUNK_USED)
			continue;

		// Ici, on a trouver un chunk libre, on vérifie sa taille (taille demandé + place pour l'en tête du chunk)
		if (current_chunk_header->size >= (size + HEAP_HEADER_SIZE))
		{
			// Ici on coupe le bloc en deux partie
			split_chunk = (struct heap_chunk_header *) ((uint32_t)current_chunk_header + HEAP_HEADER_SIZE + size);			// Espace réservé pour l'en tête du chunk
			split_chunk->magic = HEAP_HEADER_MAGIC;																			// Pour détécter les overflows
			split_chunk->size = current_chunk_header->size - size - HEAP_HEADER_SIZE;										// La taille du nouveau bloc
			list_add_after(current_chunk_header, split_chunk);																// On ajoute l'en tête après le block
			clear_bits(split_chunk->flags, CHUNK_USED);																		// Le nouveau bloc crée est disponible

			current_chunk_header->size = size;																				// On actualise la taille du bloc coupé en deux
			set_bits(current_chunk_header->flags, CHUNK_USED);																// On va retourner ce bloc, il donc maintenant utilisé 

			nb_chunks++;			// Un nouveau chunk a été crée
			nb_chunks_used++;		// Un chunk existant est désormais utilisé
			// Un chunk a été créer (et est libre) et un autre a été utilisé, donc pas besoin de toucher à nb_chunks_free
			heap_free_memory -= (current_chunk_header->size + HEAP_HEADER_SIZE);

			return (void *)(((uint32_t)current_chunk_header) + HEAP_HEADER_SIZE);											// Adresse de base du block (après l'en tête du block)
		}

	}

	// Si on arrive ici, on ne peut pas satisfaire l'allocation
	// TODO: Agrandir le heap
	return NULL;
}

/*
 * Cette fonction désalloue un bloc alloué avec kmalloc: cette fonction merge le bloc libéré
 * au bloc adjacents si possible
 *
 * TODO: implemnter fonctionnalité ci-dessus
 */
void kfree(void *obj)
{
	if (!first_chunk)
		panic("Le heap n'a pas été correctement initialisé");

	if (!obj)
		goto null_ptr;

	struct heap_chunk_header *obj_header = (struct heap_chunk_header *) ((unsigned int)obj - HEAP_HEADER_SIZE);

	// Si le nombre magique de l'en tête ne correspond pas
	if (obj_header->magic != HEAP_HEADER_MAGIC)
		goto bad_obj;								// On quitte (pas de panic, car c'est peut être l'adresse de l'objet qui est érronée)

	clear_bits(obj_header->flags, CHUNK_USED);
	heap_free_memory += obj_header->size;			// Actualise la quantité de mémoire disponible

	nb_chunks_used--;								// Un chunk a été libéré
	nb_chunks_free++;								// Un chunk a été libéré
	
	return;

	// TODO: merger les blocs adjacents disponible
null_ptr:
bad_obj :
	debugk("[HEAP]: WARNING: kfree(): Passing invalid adresse\n");
	return;
}