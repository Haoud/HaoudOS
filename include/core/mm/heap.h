#pragma once
#include <types.h>
#include <i386/paging.h>
#include <core/mm/area.h>

#define HEAP_HEADER_SIZE			sizeof(struct heap_chunk_header)
#define HEAP_HEADER_MAGIC			0xDEADC0DE
#define HEAP_INITIAL_SIZE			(PAGE_SIZE * 4)

#define CHUNK_USED					0x00000001

#define HEAP_MINIMAL_SIZE			8
#define HEAP_MAXIMAL_SIZE			((PAGE_SIZE * 4) - HEAP_HEADER_SIZE - 1)
#define HEAP_SAFE_MIN_MEM			(HEAP_HEADER_SIZE + STRUCT_VM_AREA_SIZE)		// Valeur d�termin� avec des tests vite fait: peut �tre erron�e

struct heap_chunk_header
{
	uint32_t magic;						// Nombre magique pour v�rifier les d�bordements de tampon
	uint32_t reserved;					// Pour usage ult�rieur

	flags_t flags;						// Options du chunk
	size_t size;						// Taille du chunk

	struct heap_chunk_header *next;		// Le prochain chunk
	struct heap_chunk_header *prev;		// Le chunk pr�c�dent
};

void DebugHeap(void);
void SetupHeap(vaddr_t start);
bool_t ExtendHeap(count_t nb_pages);
void AllocMemForHeap(vaddr_t start);

void *kmalloc(size_t size);
void *kmalloc_no_security(size_t size);
void kfree(void* obj);