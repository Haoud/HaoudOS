#pragma once
#include <types.h>
#include <i386/paging.h>

#define MM_CONTEXT_STRUCT_SIZE			sizeof(struct mm_context)
#define MM_CONTEXTE_USER_MEM			0x40000000

struct mm_context
{
	paddr_t pd_paddr;
	vaddr_t pd_vaddr;

	count_t ref;
	struct mm_context *prev;
	struct mm_context *next;
};

extern struct mm_context *current_mm_context;

void Setup_MMContext_Kernel(void);

struct mm_context *create_mm_context(void); 
void mm_contexte_SwitchTo(struct mm_context *to); 

void mm_context_ref(struct mm_context *context);
void mm_context_unref(struct mm_context *context);
void sync_pde_in_kernel_space(uint32_t pd_index, uint32_t pde);
struct mm_context *mm_context_duplicate(struct mm_context *context);
