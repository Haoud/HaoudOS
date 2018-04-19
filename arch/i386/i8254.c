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
#include <i386/cpu.h>
#include <i386/idt.h>
#include <i386/cpu.h>
#include <i386/tsc.h>
#include <lib/stdio.h>
#include <i386/i8254.h>
#include <i386/i8259.h>
#include <i386/ioports.h>
#include <core/time/timer.h>
#include <driver/bochs/bochs.h>
#include <core/process/process.h>
#include <core/process/schedule.h>
	
static volatile time_t sec = 0;			// Nombre de seconde depuis le démarrage du système
static volatile time_t tick = 0;		// Nombre de tick dans la seconde courante

static volatile uint32_t current_frequency;		// Fréquence du PIT actuelle
extern void asm_TimerTick(void);

/**
 * @brief Cette fonction permet d'initialiser le PIT (i8254) à la fréquence désiré qui permettera
 * de produire des interruptions à interval régulier grâce au Timer 0 du PIT
 * 
 * @param freq La fréquence du PIT désirée
 */
void i8254_SetFrenquency(unsigned int freq)
{
	if (freq <= 0)
		return;

	BochsPutc('\n');

	unsigned int ticks = PIT_8254_MAX_FRENQUENCY / freq;

	if (ticks >= 0xFFFF)
		return;
	if (ticks <= 0)
		return;

	/* Initialisation des variables utilisées par l'IRQ0 */
	current_frequency = freq;
	tick = 0;												// On réinitialise le nombre de tick

	outb(PIT_8254_CMD, 0x34);								// Configuration du Timer 0

	outb(PIT_8254_TIMER0, (uint8_t) (ticks & 0xFF));		// Envoit du 1er octet
	outb(PIT_8254_TIMER0, (uint8_t) (ticks >> 8));			// Envoit du 2nd octet

	MakeIdtDescriptor(PIT_8254_IDT_VECTOR, (uint32_t)&asm_TimerTick, 0x08, IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	i8259_EnableIrq(PIT_8254_IRQ_NR);						// Autorise l'IRQ du timer

	BochsPrintf("[INFO] Timer 0 of i8254 PIT configured to %u HZ\n", freq);
}

/**
 * @brief Cette fonction est appelé périodiquement par le PIT à une fréquence 
 * bien précise
 * 
 * @param regs Les registres du processus interrompu
 */
void TimerTick(struct cpu_state *regs)
{
	if (++tick == current_frequency)
	{
		tick = 0;
		sec++;
	}

	if(regs->cs == 0x08)
		current->kernel_tick++;
	else
		current->user_tick++;

	if(current->current_priority != 0)
		current->current_priority--;

	update_timers();						// Actualise les timers
	schedule();								// Permet de changer de tâche
	i8259_SendEOI(PIT_8254_IRQ_NR);			// Réarme le contrôleur 8259A
}

/**
 * @brief Permet d'obtenir la valeur du compteur interne du PIT
 * 
 * @return uint32_t La valeur du compteur interne du PIT
 */
uint32_t get_pit_count(void)
{
	uint32_t eflags = 0;
	uint32_t count = 0;

	lock_int(eflags);

	outbp(0x43, 0x0);
	iowait();

	count = inb(0x40);
	iowait();

	count |= (inb(0x40) << 8);
	iowait();

	unlock_int(eflags);

	return count;
}

/**
 * @brief Permet d'obtenir le nombre de nanosecondes écoulées dans la seconde courante
 * 
 * @return uint32_t Le nombre de nanosecondes écoulées dans la seconde courante
 */
uint32_t get_usec(void)
{
	uint32_t count = get_pit_count();

	count = ((PIT_8254_LATCH - 1) - count) * ((1000000UL + PIT_8254_DEFAULT_FRENQUENCY / 2) / PIT_8254_DEFAULT_FRENQUENCY);
	count = (count + PIT_8254_LATCH / 2) / PIT_8254_LATCH;

	count += tick * (10000000UL / PIT_8254_DEFAULT_FRENQUENCY);

	return count;
}


/**
 * @brief Cette fonction permet de savoir le nombre de seconde qui se sont
 * écoulé depuis le démarrage du système (plus précisément, lorsque 
 * la 1er interruption d'horloge survient)
 * 
 * @return time_t Le nombre de seconde depuis le démarrage du PC
 */
time_t get_startup_sec(void)
{
	return sec;
}

/**
 * @brief Cette fonction permet de savoir le nombre de tops d'horloge qui
 * se sont écoulé dans la seconde courante. Ils se produisent à 
 * intervalle de temps régulier (100 Hz = 100 tops d'horloge en
 * 1 seconde, donc 1 tops = 10 ms)
 * 
 * @return time_t Le nombre de tick dans la seconde courante
 */
time_t get_tick_in_this_sec(void)
{
	return tick;
}