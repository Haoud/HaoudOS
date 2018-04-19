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
#include <lib/stdio.h>
#include <lib/ctype.h>
#include <lib/string.h>
#include <driver/console.h>
#include <driver/vga/text.h>

static ecma48_state ecma_state = ECMA48_STANDBY;		// Le statut dans la récupération d'un suite de contrôle ecma 48
static ecma48_index ecma_index;							// L'index dans le tableau ecma_params où le prochain paramètre sera stocké
static ecma48_param ecma_params[ECMA48_MAXPARAM];		// Tableau où sont stockés les paramètres

#pragma GCC diagnostic ignored "-Wimplicit-fallthrough" hret_t console_write(const struct tty_device _unused *tty, char c)

hret_t console_write(const struct tty_device _unused *tty, char c)
{
	switch (ecma_state)
	{
		case ECMA48_STANDBY:
			if (c == '\033')
				ecma_state = ECMA48_ESCAPE;
			else
				Putc(c);
			break;

		case ECMA48_ESCAPE:
			ecma_state = ECMA48_STANDBY;

			if (c == '[')
				ecma_state = ECMA48_ESCAPE_CTRL;
			// TODO: gérer les séquences non CSI
			break;

		case ECMA48_ESCAPE_CTRL:
			for (int i = 0; i < ECMA48_MAXPARAM; i++)
				ecma_params[i] = 0;

			ecma_index = 0;
			ecma_state = ECMA48_GET_PARAMETERS;

		case ECMA48_GET_PARAMETERS:
			if (c == ';' && ecma_index < (ECMA48_MAXPARAM - 1))
				ecma_index++;			
			else if (is_digit(c))
				ecma_params[ecma_index] = (ecma_params[ecma_index] * 10) + ctoi(c);
			else
				ecma_state = ECMA48_PROCESSING;
			
			if (ecma_state != ECMA48_PROCESSING)
				break;

		case ECMA48_PROCESSING:
			debugk("[CONSOLE]: ECMA48 CSI sequence found\n");
			ecma_state = ECMA48_STANDBY;

			switch (c)
			{
				case 'K':
					switch(ecma_params[0])
					{
						case 0:
							ClearLine(FROM_CURSOR);
							break;

						case 1:
							ClearLine(TO_CURSOR);
							break;

						case 2:
							ClearLine(WHOLE_LINE);
							break;

						default:
							break;
					}

					break;

				case 'H':
					// Note: Pour le standart ECMA 48, l'origine est en 1,1; Pour le driver, c'est 0, 0
					if (ecma_params[0])			// Si l'absisse n'est pas nul
						ecma_params[0]--;		// On le met au standart du driver écran

					if (ecma_params[1])			// Si l'ordonnée n'est pas nul
						ecma_params[1]--;		// On le met au standart du driver écran

					GoTo(ecma_params[0], ecma_params[1]);
					break;

				case 'J':						// Effacement de l'écran
					if (ecma_params[0] == 1)
						debugk("[CONSOLE]: Unsupported SCI sequence 1J: using 2J");

					ClearScreen();
					break;

				case 'm':						// Changer l'attribut vidéo des prochains caractères affichés
					csi_m();
					break;

				case 'd':						// Se déplacer dans la colonne indiquée, même ligne
					GoTo(ecma_params[0], -1);
					break;

				case '\'':						// Se déplacer dans la ligne indiquée, même colonne
					GoTo(-1, ecma_params[0]);
					break;

				default:
					// Si la séquence n'est pas reconnue alors on l'affiche à l'écran
					printk("\033[");
					for(ecma48_index i = 0; i < ecma_index; i++)
					{
						printk("%u", ecma_params[i]);
						if(i < (ecma_index - 1))
							printk(";");
					}

					printk("%c", c);
					break;
			}

			// TODO: Gérer d'autre séquence CSI
			break;

		default:
			ecma_state = ECMA48_STANDBY;
			break;
	}

	return RET_OK;
}

void csi_m(void)
{
	int high_intensity = 0;

	for (ecma48_index i = 0; i < (ecma_index + 1); i++)
	{
		switch (ecma_params[i])
		{
			case 7:
				high_intensity = 8;
				break;

			default:
				break;
		}
	}

	for (ecma48_index i = 0; i < (ecma_index + 1); i++)
	{
		switch (ecma_params[i])
		{
			case 0:
				SetTextColor(LIGHT_GREY, BLACK);
				break;

			case 30:
				SetTextColor(BLACK, NO_CHANGE);
				break;
			case 31:
				SetTextColor(RED + high_intensity, NO_CHANGE);
				break;
			case 32:
				SetTextColor(GREEN + high_intensity, NO_CHANGE);
				break;
			case 33:
				SetTextColor(YELLOW, NO_CHANGE);
				break;
			case 34:
				SetTextColor(BLUE + high_intensity, NO_CHANGE);
				break;
			case 35:
				SetTextColor(MAGENTA + high_intensity, NO_CHANGE);
				break;
			case 36:
				SetTextColor(CYAN + high_intensity, NO_CHANGE);
				break;
			case 37:
				SetTextColor(WHITE, NO_CHANGE);
				break;


			case 40:
				SetTextColor(NO_CHANGE, BLACK);
				break;
			case 41:
				SetTextColor(NO_CHANGE, RED);
				break;
			case 42:
				SetTextColor(NO_CHANGE, GREEN);
				break;
			case 43:
				SetTextColor(NO_CHANGE, YELLOW);
				break;
			case 44:
				SetTextColor(NO_CHANGE, BLUE);
				break;
			case 45:
				SetTextColor(NO_CHANGE, MAGENTA);
				break;
			case 46:
				SetTextColor(NO_CHANGE, CYAN);
				break;
			case 47:
				SetTextColor(NO_CHANGE, WHITE);
				break;

			default:
				break;
		}
	}
}
