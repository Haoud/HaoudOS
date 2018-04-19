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

#define MASTER_PIC_CMD		0x20			// Port de commande du PIC 8259 maître
#define MASTER_PIC_DATA		0x21			// Port de données du PIC 8259 maître

#define SLAVE_PIC_CMD		0xA0			// Port de commande du PIC 8259 esclave
#define SLAVE_PIC_DATA		0xA1			// Port de données du PIC 8259 esclave

#define I8259_EOI			0x20			// Commande de fin d'interruption (accusé de récéption)

#define ICW1_NEED_ICW4		0x01			// Port ICW1 - Port ICW4 requis pour l'initialisation
#define ICW1_SINGLE_MODE	0x02			// Mode simple (sinon mode cascade)
#define ICW1_INTERVAL_4		0x04			// Intervale dans l'IDT de 4 (sinon de 8)
#define ICW1_LEVEL			0x08			// Déclenchement par niveau (sinon par "front")
#define ICW1_INIT_REQUIRED	0x10			// Initialisation requis

#define ICW4_8086			0x01			// Processeur 80806 et ses descendants (i386, i586...)
#define ICW4_EOI_AUTO		0x02			// End of interrupt automatique (manuel sinon)
#define ICW4_BUF_SLAVE		0x08			// Mode buffer pour le PIC esclave
#define ICW4_BUF_MASTER		0x0C			// Mode buffer pour le PIC maitre
#define ICW4_SPECIAL_FULLY	0x10			// Mode SPECIAL_FULLY 

void i8259_AutoRemap(void);
void i8259_EnableAllIrq(void);
void i8259_DisableAllIrq(void);
void i8259_SendEOI(const int IrqNr);
void i8259_EnableIrq(const int IrqNr);
void i8259_DisableIrq(const int IrqNr);

uint16_t i8259_SaveMask(void);
void i8259_RestoreMask(uint16_t saved_mask);



