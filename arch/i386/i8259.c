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
#include <i386/i8259.h>
#include <i386/ioports.h>
#include <driver/bochs/bochs.h>

/*
 * Cette fonction remappe automatiquement les IRQs au vecteurs d'interruptions de l'IDT 0x20 pour 
 * L'IRQ0, 0x21 pour l'IRQ1... et en profite pour initialiser le PIC correctement
 * Cette fonction interdit l'émission de toutes les IRQs afin de ne pas provoquer un crash du
 * noyau: les drivers noyau utilisant ces IRQs doivent réactiver manuellement les IRQs
 */
void i8259_AutoRemap(void)
{
	// Initialisation des 2 PICs en mode cascade, interval de 8 octet dans l'IDT, 
	// déclenchement par front, ICW4 requis pour l'initialisation
	outbp(MASTER_PIC_CMD, ICW1_NEED_ICW4 | ICW1_INIT_REQUIRED);
	outbp(SLAVE_PIC_CMD, ICW1_NEED_ICW4 | ICW1_INIT_REQUIRED);

	iowait();

	// Remappage des IRQs dans les vecteurs d'interruptions 0x20 jusqu'a 0x2F de l'IDT
	outbp(MASTER_PIC_DATA, 0x20);
	outbp(SLAVE_PIC_DATA, 0x28);

	iowait();

	outbp(MASTER_PIC_DATA, 4);		// Informer au PIC maître que un PIC esclave est relié à l'IRQ 2
	outbp(SLAVE_PIC_DATA, 2);		// Informer au PIC esclave qu'il est relié à l'IRQ 2 du PIC maître

	iowait();

	// Initialise du PIC pour les processeur 8086 et ses descendants (i386, i486, i586...)
	outbp(MASTER_PIC_DATA, ICW4_8086);
	outbp(SLAVE_PIC_DATA, ICW4_8086);

	BochsPrint("\n[INFO] i8259A PIC initialized and IRQs are remapped to vector 0x20:0x2F\n");
	BochsPrint("[INFO] Slave PIC connected to IRQ2 of Master PIC\n");
	BochsPrint("[INFO] For security, all IRQs are disable: manually enable required\n");

	i8259_DisableAllIrq();			// Interdire toutes les IRQs non prévu
}

/*
 * Active toutes les interruptions "IRQs"
 */
void i8259_EnableAllIrq(void)
{
	uint8_t mask = 0;				// Masque "vide" = autoriser toutes les IRQs
	outbp(SLAVE_PIC_DATA, mask);
	outbp(MASTER_PIC_DATA, mask);

	BochsPrint("[INFO] All IRQs enabled\n");
}

/*
 * Désacive toutes les interruptions "IRQs"
 */
void i8259_DisableAllIrq(void)
{
	uint8_t mask = 0xFF;			// Masque "plein" = interdire toutes les IRQs
	outbp(SLAVE_PIC_DATA, 0xFB);	// Autorise quand même le PIC en cascade
	outbp(MASTER_PIC_DATA, mask);

	BochsPrint("[INFO] All IRQs disabled\n");
}

/*
 * Cette fonction permet d'envoyer au PIC une requète de fin d'interruption (EOI).
 * Si l'interruption utilise le PIC esclave, cette fonction se charge aussi 
 * d'envoyer l'EOI au PIC esclave mais dans tout les cas, elle envoie un EOI au
 * PIC maître (Rappel: PIC esclave connecté à l'IRQ 2 du PIC maître)
 */
void i8259_SendEOI(const int IrqNr)
{
	if (IrqNr > 8)
		outbp(SLAVE_PIC_CMD, I8259_EOI);

	outbp(MASTER_PIC_CMD, I8259_EOI);
}

/*
 * Cette fonction permet de masquer une ligne d'IRQ au processeur simplement en définisant
 * à 1 le bit correspondant à la ligne d'IRQ du PIC (Rappel: 16 IRQ pour 2 PICs = 8 IRQ
 *  par PIC = un octet de masquage)
 * Le masque actuel des lignes d'IRQ est contenu dans le port IO du PIC correspondant
 * (IRQ < 8 = PIC maître, IRQ > 8 = PIC esclave)
 * 
 * NOTE: l'interruption masquée n'est pas perdu, elle est envoyée au processeur dès que 
 * possible, c'est à dire dès que la ligne d'IRQ soit réactivée
 */
void i8259_DisableIrq(const int IrqNr)
{
	uint8_t current_mask = 0;

	if (IrqNr > 8)
	{
		current_mask = inb(SLAVE_PIC_DATA);			// Récupértation du masque actuel des lignes masquées du PIC
		current_mask |= (1 << (IrqNr - 8));			// On modifie le bit désiré des lignes masquées du PIC
		outbp(SLAVE_PIC_DATA, current_mask);		// Puis on renvoie ce masque au PIC
	}
	else
	{
		current_mask = inb(MASTER_PIC_DATA);		// Récupértation du masque actuel des lignes masquées du PIC
		current_mask |= (1 << IrqNr);				// On modifie le bit désiré des lignes masquées du PIC
		outbp(MASTER_PIC_DATA, current_mask);		// Puis on renvoie ce masque au PIC
	}

	BochsPrintf("[DEBUG] IRQ line %u disabled\n", IrqNr);
}

/*
 * Cette fonction permet de démasquer une ligne d'IRQ au processeur grâce
 * au PIC. Pour plus d'information voir le commentaire de la fonction ci
 * dessus :)
 */
void i8259_EnableIrq(const int IrqNr)
{
	uint8_t current_mask = 0;

	if (IrqNr > 8)
	{
		current_mask = inb(SLAVE_PIC_DATA);			// Récupértation du masque actuel des lignes masquées du PIC
		current_mask &= ~(1 << (IrqNr - 8));		// On modifie le bit désiré des lignes masquées du PIC
		outbp(SLAVE_PIC_DATA, current_mask);		// Puis on renvoie ce masque au PIC
	}
	else
	{
		current_mask = inb(MASTER_PIC_DATA);		// Récupértation du masque actuel des lignes masquées du PIC
		current_mask &= ~(1 << IrqNr);				// On modifie le bit désiré des lignes masquées du PIC
		outbp(MASTER_PIC_DATA, current_mask);		// Puis on renvoie ce masque au PIC
	}

	BochsPrintf("[DEBUG] IRQ line %u enabled\n", IrqNr);
}

uint16_t i8259_SaveMask(void)
{
	uint16_t mask = 0;
	mask = (inb(MASTER_PIC_DATA) << 8);
	mask |= inb(SLAVE_PIC_DATA) ;

	return mask;
}

void i8259_RestoreMask(uint16_t saved_mask)
{
	outbp(SLAVE_PIC_DATA, (saved_mask & 0xFF));
	outbp(MASTER_PIC_DATA, ((saved_mask << 8) & 0xFF));
}