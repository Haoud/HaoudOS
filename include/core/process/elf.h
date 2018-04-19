#pragma once
#include <types.h>

#define PT_LOAD    1

struct ElfHeader
{
	uint8_t signature[16];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint32_t entry;
	uint32_t program_table_off;
	uint32_t section_table_off;
	uint32_t flags;
	uint16_t header_size;
	uint16_t program_table_size;
	uint16_t program_table_nb_entries;
	uint16_t section_table_size;
	uint16_t section_table_nb_entries;
	uint16_t string_table_index;
}_packed;

struct ElfProgramHeader
{
	uint32_t type;
	uint32_t offset;
	uint32_t vaddr;
	uint32_t paddr;
	uint32_t file_size;
	uint32_t mem_size;
	uint32_t flags;
	uint32_t align;
}_packed;

hret_t LoadElfFile(const char *pathname, uint32_t *eip, struct process *p);
