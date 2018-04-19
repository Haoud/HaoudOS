#pragma once
#include <types.h>
#include <i386/paging.h>
#include <core/process/process.h>

#define USER_HEAP_DEFAULT_SIZE		PAGE_SIZE

uint32_t extend_brk(void *addr);
void init_brk(struct process *p);
