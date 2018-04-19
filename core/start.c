#include <boot.h>
#include <types.h>
#include <core/main.h>
#include <lib/stdlib.h>

static struct HaoudOSBootInfo boot_struct;
static struct HaoudOSBootInfo *boot_ptr = &boot_struct;

static const size_t boot_struct_size = sizeof(struct HaoudOSBootInfo);

/*
 * Cette fonction est la premi�re a �tre appel�e par le bootloader, c'est pour cela qu'elle est
 * un peu "exotique" car elle initialise le contexte pour les autres fonctions qui vont �tre 
 * appel�e par la suite
 *
 * Elle re�oit en param�tre un nombre magique pour v�rifier que c'est bien le loader d'HaoudOS qui
 * est utilis� (car j'utilise ma propre structure de boot) et la structure de boot proprement dites.
 * Comme la structure de boot est situ� dans la m�moire basse (< 1MO), je la recopie dans un 
 * emplacement pr�d�finit dans le binaire du noyau pour qu'elle ne soit pas corrompu par la suite
 *
 * La pile a d�j� �t� initialis� par le loader donc on n'y touche pas pour l'instant
 */
void __attribute__((section("entry"))) _entry(const uint32_t magic, const struct HaoudOSBootInfo *BootData)
{
	asm volatile("cli																\n\
		mov ax, 0x10																\n\
		mov ds, ax																	\n\
		mov es, ax																	\n\
		mov fs, ax																	\n\
		mov gs, ax																	\n\
		mov ss, ax																	\n\
		cmp [%1], 0																	\n\
		je bad_loader																\n\
		jmp loader_ok																\n\
	bad_loader:																		\n\
		jmp $																		\n\
	loader_ok:																		\n\
		push DWORD ptr %2															\n\
		push DWORD ptr [%3]															\n\
		push DWORD ptr [%0]															\n\
		call memcpy																	\n\
		add esp, 12																	\n\
		push DWORD ptr [%0]															\n\
		push DWORD ptr [%4]															\n\
		call startup32																\n\
		add esp, 8																	\n\
	idle:																			\n\
		hlt																			\n\
		jmp idle" : "+m"(boot_ptr) : "m"(BootData), "i"(boot_struct_size), "m"(BootData), "m"(magic) : "memory", "eax", "ebx", "edx", "esi", "edi", "esp", "cc");
}