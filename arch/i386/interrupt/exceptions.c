/*
 * This file was created on Tue Apr 17 2018
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
#include <i386/cpu.h>
#include <i386/idt.h>
#include <lib/stdio.h>
#include <i386/exceptions.h>
#include <core/syscall/syscall.h>

extern void asm_page_fault(void);
extern void asm_divide_by_zero(void);
extern void asm_debug(void);
extern void asm_nmi(void);
extern void asm_breakpoint(void);
extern void asm_overflow(void);
extern void asm_bound(void);	
extern void asm_invalide_op(void);		
extern void asm_double_fault(void);			
extern void asm_coprocessor_error(void);	
extern void asm_invalide_tss(void);
extern void asm_segment_not_present(void);
extern void asm_stack_fault(void);
extern void asm_general_protection_fault(void);
extern void asm_fpu_exception(void);

void do_page_fault(struct cpu_error_state *regs);

/**
 * @brief Cette fonction se charge d'initialiser les exceptions du processeur x86 avec
 * les handlers par défaut. Mais ces handler peuvent être remplacé par d'autre fonctions,
 * qui, typiquement, vont surcharger page_fault et general_protection_fault afin que
 * ces exceptions ne fasse plus planter le noyau
 */
void SetupExceptions(void)
{
	MakeIdtDescriptor(0, (uint32_t)asm_divide_by_zero,				0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	MakeIdtDescriptor(1, (uint32_t)asm_debug,						0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	MakeIdtDescriptor(2, (uint32_t)asm_nmi,							0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	MakeIdtDescriptor(3, (uint32_t)asm_breakpoint,					0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT | IDT_USER_ACCES);			// Accesible à l'utilisateur
	MakeIdtDescriptor(4, (uint32_t)asm_overflow,					0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT | IDT_USER_ACCES);			// Accesible à l'utilisateur
	MakeIdtDescriptor(5, (uint32_t)asm_bound,						0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT | IDT_USER_ACCES);			// Accesible à l'utilisateur
	MakeIdtDescriptor(6, (uint32_t)asm_invalide_op,					0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	// Cette interruption doit être géré par la partie FPU d'HaoudOS (interruption)
	MakeIdtDescriptor(8, (uint32_t)asm_double_fault,				0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	MakeIdtDescriptor(9, (uint32_t)asm_coprocessor_error,			0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	MakeIdtDescriptor(10, (uint32_t)asm_invalide_tss,				0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	MakeIdtDescriptor(11, (uint32_t)asm_segment_not_present,		0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	MakeIdtDescriptor(12, (uint32_t)asm_stack_fault,				0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	MakeIdtDescriptor(13, (uint32_t)asm_general_protection_fault,	0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	MakeIdtDescriptor(14, (uint32_t)asm_page_fault,					0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	MakeIdtDescriptor(16, (uint32_t)asm_fpu_exception,				0x08,	IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
}

void nmi(struct cpu_state _unused *regs)
{
	panic("Interruption non-masquable survenue");
}

void debug(struct cpu_state _unused *regs)
{
	debugk("[EXCEPTION]: Unsupported debug interruption\n");
}

void bound(struct cpu_state _unused *regs)
{
	debugk("[EXCEPTION]: Unsupported bound interruption\n");
}

void overflow(struct cpu_state _unused *regs)
{
	debugk("[EXCEPTION]: Unsupported overflow interruption\n");
}

void breakpoint(struct cpu_state _unused *regs)
{
	debugk("[EXCEPTION]: Unsupported breakpoint interruption\n");
}

void page_fault(struct cpu_error_state *regs)
{
	do_page_fault(regs);
}

void stack_fault(struct cpu_error_state _unused *regs)
{
	panic("Erreur de la pile");
}

void invalide_op(struct cpu_state *regs)
{
	panic("Code d'opération invalide en eip %08x", regs->eip);
}

void double_fault(struct cpu_error_state _unused *regs)
{
	panic("Double faute");
}

void invalide_tss(struct cpu_error_state _unused *regs)
{
	panic("TSS invalide");
}

void fpu_exception(struct cpu_state _unused *regs)
{
	set_signal(current->signal, SIGFPE);
}

void divide_by_zero(struct cpu_state _unused *regs)
{
	set_signal(current->signal, SIGFPE);
}

void coprocessor_error(struct cpu_state _unused *regs)
{
	// Que faire ?
}

void segment_not_present(struct cpu_error_state _unused *regs)
{
	panic("Segment non présent");
}

void general_protection_fault(struct cpu_error_state _unused *regs)
{
	panic("Faute de protection générale");
}