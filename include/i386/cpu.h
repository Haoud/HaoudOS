/*
 * This file was created on Wed Mar 28 2018
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
#pragma once
#include <types.h>

#define in_user_mode(cs)	(cs != 0x08)

#define set_ts() 					\
	asm volatile("mov eax, cr0 	\n	\
				  or al, 0x0008	\n	\
				  mov cr0, eax" ::: "eax")

// Contexte CPU sauvegardé par le CPU et les gestionnaires d'interruptions
struct cpu_state
{
	uint32_t ss;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
}_packed;


// Contexte CPU sauvegardé par le CPU lors d'une exception avec un code d'erreur
struct cpu_error_state
{
	uint32_t ss;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	uint32_t error_code;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;

}_packed;

struct cpu_user_state
{
	struct cpu_state regs;
	uint32_t esp3;
	uint32_t ss3;
}_packed;

struct cpu_user_error_state
{
	struct cpu_error_state regs;
	uint32_t esp3;
	uint32_t ss3;
}_packed;


struct kernel_stack
{
	uint32_t esp0;
	uint16_t ss0;
};

struct cpuid_result
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
};

#define HANDLER_SAVE_REGS		\
		asm("pushad				\n\
		push ds					\n\
		push es					\n\
		push fs					\n\
		push gs					\n\
		push ss					\n\
								\n\
		push ax					\n\
		mov ax, 0x10			\n\
		mov ds, ax				\n\
		mov es, ax				\n\
		mov fs, ax				\n\
		mov gs, ax				\n\
		mov ss, ax				\n\
		pop ax" ::: "eax", "ebx", "ecx", "edx", "edi", "esi");
	

#define HANDLER_RESTORE_REGS	\
		asm("pop ss				\n\
		 	 pop gs 			\n\
			 pop fs				\n\
			 pop es				\n\
			 pop ds				\n\
			 popad" ::: "eax", "ebx", "ecx", "edx", "edi", "esi");			

#define cli()	asm("cli")
#define sti()	asm("sti")
#define hlt()	asm("hlt")
#define nop()	asm("nop")

#define	EFLAGS_CF			0x00000001				// Débordement sur opération non signé ou une opération arithmétique génère une retenue sur le bit de poids fort
#define	EFLAGS_PF			0x00000004				// Armé si l'octet de poids faible du résultat généré après une opération arithmétique contient un nombre pair de bits à 1
#define	EFLAGS_AF			0x00000010				// Armé si le résultat d'une opération arithmétique génère un résultat provoquant une retenue sur le troisième bit (Pas très utile...)
#define	EFLAGS_ZF			0x00000040				// Armé si le résultat d'une opération arithmétique vaut zéro
#define	EFLAGS_SF			0x00000080				// Armé si le résultat d'une opération arithmétique possède un bit de poids fort à 1, ce qui indique un nombre signé
#define	EFLAGS_TF			0x00000100				// Permet d'activer le débogage pas à pas si le drapeau est armé
#define	EFLAGS_IF			0x00000200				// Permet d'activer les interruptions
#define	EFLAGS_DF			0x00000400				// Armé si les chaînes de caractères sont auto-décrémenté, sinon elle sont auto-incrémenté
#define	EFLAGS_OF			0x00000800				// Armé si le résultat constitue un nombre positif ou négatif (en excluant le bit de signe) ne pouvant tenir dans l'opérande de destination
#define	EFLAGS_IOPL			0x00003000				// Permet d'activer la vérification des accès aux ports i/O (permission dans le TSS)
#define	EFLAGS_IOPL_KERNEL	0x00000000				// On utilise les ports I/O en mode noyau
#define	EFLAGS_IOPL_USER	0x00003000				// On utilise les ports I/O en mode utilisateur
#define	EFLAGS_NT			0x00004000				// La tâche est-elle un tâche chainée ?
#define	EFLAGS_RF			0x00010000				// Utilisé pour le débogage pas à pas
#define	EFLAGS_VM			0x00020000				// Lorsque ce drapeau est activé le processeur est en mode virtuel
#define EFLAGS_AC			0x00040000				// Lorsque ce drapeau est positionné, le processeur vérifie l'alignement (nécessite modification cr0)
#define EFLAGS_VIF			0x00080000				// Ce drapeau est une image virtuelle du drapeau IF d'EFLAGS
#define EFLAGS_VIP			0x00100000				// Interruption en attente (mode virtuel 8086)
#define EFLAGS_ID			0x00200000				// Le processeur supporte t'il l'instruction CPUID ?

#define get_eflags(var)									\
{														\
	asm("pushf;pop %0" : "=r"(var) :: "memory");		\
}


#define set_eflags(var)									\
{														\
	asm("push %0;popf" :: "r"(var) : "memory", "cc");	\
}

#define save_eflags(var) 	get_eflags(var)
#define restore_eflags(var) set_eflags(var)

#define lock_int(f)			save_eflags(f);cli()
#define unlock_int(f)		restore_eflags(f)

#define CPUID_CMD_GET_VENDOR_ID				0x00
#define CPUID_CMD_GET_FEATURES				0x01
#define CPUID_CMD_GET_CACHE_INFO			0x02
#define CPUID_CMD_GET_SERIAL_NUM			0x03
#define CPUID_CMD_GET_TOPOLOGY				0x04

// Feature de la commande CPUID_CMD_GET_FEATURES dans EDX
#define CPUID_FEATURE_FPU						0x00000000
#define CPUID_FEATURE_VM8086_EXTENTION			0x00000001
#define CPUID_FEATURE_DEBUGGING_EXTENTION		0x00000002
#define CPUID_FEATURE_PSE_SUPPORTED				0x00000004
#define CPUID_FEATURE_TSC_PRESENT				0x00000008
#define CPUID_FEATURE_MSR_PRESENT				0x00000010
#define CPUID_FEATURE_PAE_SUPPORTED				0x00000020
#define CPUID_FEATURE_MCE_SUPPORTED				0x00000040
#define CPUID_FEATURE_CX8_INSTRUCTION			0x00000080
#define CPUID_FEATURE_APIC_PRESENT				0x00000100
#define CPUID_FEATURE_SEP_INSTRUCTION			0x00000400
#define CPUID_FEATURE_MTRR_REGISTER				0x00000800
#define CPUID_FEATURE_PGE_SUPPORTED				0x00001000
#define CPUID_FEATURE_MCA_ARCHITECTURE			0x00002000
#define CPUID_FEATURE_CMOV_INSTRUCTION			0x00004000
#define CPUID_FEATURE_PAT_SUPPORTED				0x00008000
#define CPUID_FEATURE_PSE_36_SUPPORTED			0x00010000
#define CPUID_FEATURE_PSN						0x00020000
#define CPUID_FEATURE_CLFLUSH_SUPPORTED			0x00040000
//TODO: Compléter la liste
#define CPUID_FEATURE_MMX_INSTRUCTION			0x00800000
#define CPUID_FEATURE_FXSR_INSTRUCTION			0x01000000
#define CPUID_FEATURE_SSE_INSTRUCTION			0x02000000
#define CPUID_FEATURE_SSE2_INSTRUCTION			0x04000000


// Feature de la commande CPUID_CMD_GET_FEATURES dans ECX
//TODO: Compléter la liste

void cpuid_init(void);
bool_t cpuid(uint32_t cmd, struct cpuid_result *res);