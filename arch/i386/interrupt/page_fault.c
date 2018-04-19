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
#include <types.h>
#include <i386/cpu.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <core/mm/mm_context.h>
#include <core/process/signal.h>
#include <core/process/process.h>
#include <core/syscall/syscall.h>

/**
 * @brief Cette fonction est appelé lorsque la MMU n'est pas capable de faire la traduction
 * adresse virtuelle -> adresse physique (Accès interdit, adresse virtuelle invalide...)
 * 
 * Cette fonction est chargé de résoudre se problème si possible (en étend la pile, en allouant
 * des pages pour le heap...). Cette fonction peut échouer et tuer le processus en cours si la
 * faute de page n'est pas résolvable (Bug du processus mais parfois du noyau)
 * 
 * @param regs Les registre du processus (ou du noyau) interrompu par la faute de page
 */
void do_page_fault(struct cpu_user_error_state *regs)
{
    uint32_t fautive_address;
    uint32_t pages_to_extent;
    uint32_t stack_addr_extended;

    bool_t _unused protection_violation = regs->regs.error_code & 0x01;         // Défaut de page par violation d'accès ?
    bool_t _unused write_acces = regs->regs.error_code & 0x02;                  // Accès en écriture ou en lecture ?
    bool_t user_mode = regs->regs.error_code & 0x04;                            // En mode utilisateur ou en mode noyau ?
    
    asm("mov eax, cr2	\n\
         mov %0, eax" : "=g"(fautive_address) :: "eax");						// Récupération de l'adresse fautive

    // Si la faute de page concerne un processus utilisateur qui tente d'accèder à l'espace du noyau
    if(user_mode && fautive_address < MM_CONTEXTE_USER_MEM)                     
        goto bad_area;

    // Si la faute de page se trouve dans la zone alloué pour le heap
    if(fautive_address >= (uint32_t)current->brk_start && fautive_address <= (uint32_t)current->brk_end)
        goto extend_heap;

    // Si la faute de page se trouve dans la zone alloué pour la pile (avec une marge de 32 octets)
    if(fautive_address + 32 >= regs->esp3)
        goto extend_stack;

    // Si la faute de page a eu lieu en mode noyau
    if(!user_mode)
        goto kernel_oops;

    goto bad_area;        

/* Utilitaire de gestion des fautes de pages */

kernel_oops:
    debugk("Fatal kernel page fault at eip %08x for adress %08x\n", regs->regs.eip, fautive_address);
    printk("Oops: HaoudOS has encountered an error and must close this application !\n");
    sys_exit(-1);																							// Tue le processus en cours

extend_heap:
    PagingAutoMap(fautive_address & PAGE_MASK, 1, VM_MAP_ATOMIC | VM_MAP_USER | VM_MAP_WRITE);				// Alloue une pages physique et la mappe
    memset((void *)(fautive_address & PAGE_MASK), 0, PAGE_SIZE);											// Met le contenu de la page à zéro
    return;

extend_stack:
    stack_addr_extended = fautive_address & PAGE_MASK;														// Détecte le haut de la pile
    pages_to_extent = ((uint32_t)current->stack_end - (uint32_t)stack_addr_extended) / PAGE_SIZE;			// Dtermine le nombre de pages qui faut allouer
    PagingAutoMap(stack_addr_extended, pages_to_extent, VM_MAP_ATOMIC | VM_MAP_USER | VM_MAP_WRITE);		// Mappe toute les pages qui n'ont pas été alloué
    memset((void *)(fautive_address & PAGE_MASK), 0, PAGE_SIZE * pages_to_extent);							// Met le contenu des page à zéro

	current->stack_end = (void *)stack_addr_extended;			// Met à jour la fin de la pile
    return;

bad_area:
    debugk("[SISSEG]: Process pid %u do a illegal page fault at %08x (EIP: %08x)\n", current->pid, fautive_address, regs->regs.eip);
    set_signal(current->signal, SIGSEGV);																	// Envoie un signal SIGSEGV au processus (ce qui le tue généralement)
    return;
}