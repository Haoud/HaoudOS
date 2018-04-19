#include <lib/stdio.h>
#include <lib/string.h>
#include <lib/stdlib.h>
#include <core/shell.h>
#include <i386/ioports.h>
#include <driver/bochs/bochs.h>
#include <driver/keyboard/keyboard.h>

static struct cmd_struct cmd_fn_tab[SHELL_NR_CMD];
static bool_t exiting = FALSE;

void start_shell(void)
{
	shell_init();
	shell_loop();
}

void shell_init(void)
{
	cmd_fn_tab[0].cmd_name = "exit";
	cmd_fn_tab[0].description = "Quitte l'interpr�teur de commande";
	cmd_fn_tab[0].cmd_fn = _exit;

	cmd_fn_tab[1].cmd_name = "echo";
	cmd_fn_tab[1].description = "Affiche un message � l'�cran";
	cmd_fn_tab[1].cmd_fn = echo;

	cmd_fn_tab[2].cmd_name = "help";
	cmd_fn_tab[2].description = "Affiche la liste des commandes (Ah bon ?)";
	cmd_fn_tab[2].cmd_fn = help;

	cmd_fn_tab[3].cmd_name = "reboot";
	cmd_fn_tab[3].description = "Red�marre l'ordinateur";
	cmd_fn_tab[3].cmd_fn = reboot;

	cmd_fn_tab[4].cmd_name = "bprint";
	cmd_fn_tab[4].description = "Affiche un message sur le port 0xe9 de Bochs";
	cmd_fn_tab[4].cmd_fn = bprint;
}

void shell_loop(void)
{
	char cmd_buf[128];			// Buffer pour la commande
	char *cmd_start;			// Pointeur sur le d�but r�el de la commande
	int i = 0;					// Variable utilis� par les boucles

	while (!exiting)			// Tant que l'on ne souhaite pas quitter le shell
	{
		clear_buf(cmd_buf, 128);		// Effacement de la pr�c�dente commande

		printk("\n%s", SHELL_CMD);		// Affichage du shell
		input_line(cmd_buf, 128);		// R�cup�re une ligne
		
		// Enl�ve le retour � la ligne
		for (i = 0; i < 128; i++)
		{
			if (cmd_buf[i] == '\n')
			{
				cmd_buf[i] = 0;
				break;
			}
		}

		// Enl�ve les blancs avant la commande
		cmd_start = cmd_buf;
		for (i = 0; i < 128 && cmd_buf[i] == ' ' ; i++)
			cmd_start++;

		// Cherche la commande correspondante
		for (i = 0; i < SHELL_NR_CMD; i++)
		{
			if (strncmp(cmd_start, 
				        cmd_fn_tab[i].cmd_name, 
						strlen(cmd_fn_tab[i].cmd_name)) == 0)
			{
				cmd_fn_tab[i].cmd_fn(cmd_start + strlen(cmd_fn_tab[i].cmd_name));		// Appel la fonction avec les arguments fourni apr�s la commande
				break;
			}
		}

		// Si la commande est inconnue...
		if (i == SHELL_NR_CMD)
			printk("Commande inconnue. Tapez help pour plus d'information\n");
	}
}

/*
 * Cette fonction, utiliser par le shell, permet de r�cup�rer une ligne d'une taille
 * maximum sp�cifi� par l'utilisateur et et stock� dans le tampon buf. Cette fonction
 * g�re seulement comme caract�re sp�cial le retour arri�re et ne supprime pas le 
 * � la ligne
 */
void input_line(char *buf, size_t max_char)
{
	char c = 0;
	size_t num_char = 0;

	do
	{
		//c = getch();		// R�cup�re un caract�re

		if (c == 0x1B)		// Si ce caract�re introduit un caract�re sp�cial
		{
			//c = getch();	// On le lit
			switch (c)		// Puis on l'interpr�te
			{
				/* Retour arri�re */
				case 0x07:
					if (num_char > 0)
					{
						buf[num_char--] = 0;
						printk("\b \b");
					}
					break;

				default:
					break;
			}
		}
		else
		{
			// Si on ne risque pas de d�passer le tampon
			if (num_char < (max_char - 1))
			{
				buf[num_char] = c;
				printk("%c", c);

				num_char++;
			}
		}

	} while (c != '\n');

	buf[num_char] = 0;		// Ajoute un caract�re nul
}

/*
 * Fonction: exit
 * Description: Quitte l'interpr�tateur de commande
 */
void _exit(char *buf)
{
	buf = buf;
	exiting = TRUE;
}

/*
 * Fonction: echo
 * Description: Affiche un message a l'�cran � partir du premier
 * caract�re alpha-num�rique rencontr�
 */
void echo(char *buf)
{
	while (*buf == ' ' && *buf != 0)
		buf++;

	if (*buf == 0)
		printk("Utilisation: echo <texte � afficher>\n");

	printk("%s\n", buf);
}

/*
 * Fonction: help
 * Description: Affiche une description basique de chaque commande
 * disponible dans le shell
 */
void help(char *buf)
{
	// Affiche la description de toutes les commandes
	buf = buf;
	for (int i = 0; i < SHELL_NR_CMD; i++)
	{
		printk("\t%s: %s\n", cmd_fn_tab[i].cmd_name, cmd_fn_tab[i].description);
	}
}

void reboot(char *buf)
{
	buf = buf;

	uint8_t kbdBufferFull = 0x02;
	while (kbdBufferFull & 0x02)
		kbdBufferFull = inb(0x64);

	outb(0x64, 0xFE);
}

void bprint(char *buf)
{
	while (*buf == ' ')
		buf++;

	BochsPrint(buf);
}