/*
 * This file was created on Tue Mar 27 2018
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
#include <assert.h>
#include <i386/idt.h>
#include <i386/interrupt.h>
#include <driver/bochs/bochs.h>

static struct IdtDescriptor Idt[IDT_SIZE];		// Les entrées des interruptions (255 au total)
static struct IdtRegister Idtr;					// Le registre pointant sur l'IDT et indiquant sa taille

extern void asm_default_handler(void);

/*
 * Cette fonction génère l'IDT par défaut et initialise le registre IDTR
 */
void GenerateIDT(void)
{
	BochsPrintf("\n[DEBUG] Adresse of IDT: 0x%p\n", &Idt);
	BochsPrintf("[DEBUG] Location of IDT Register: 0x%p\n", &Idtr);
	
	for (int i = 0; i < IDT_SIZE; i++)
		MakeIdtDescriptor(i, (uint32_t) &asm_default_handler, 0x08, IDT_INT_GATE32 | IDT_HANDLER_PRESENT);

	Idtr.base = (uint32_t)&Idt;
	Idtr.limit = IDT_SIZE * IDT_DESC_SIZE;

	FlushIdtr();
}

void FlushIdtr(void)
{
	asm("lidt [Idtr]");
}

void MakeIdtDescriptor(uint32_t index, uint32_t fn, uint16_t selector, uint8_t acces)
{
	if (index >= IDT_SIZE)
		return;

	struct IdtDescriptor *ptr = &Idt[index];

	ptr->offset0_15 = (fn & 0xFFFF);
	ptr->offset15_31 = ((fn >> 16) & 0xFFFF);

	ptr->selector = selector;
	ptr->unused = 0;
	ptr->acces = acces;
}