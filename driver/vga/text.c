#include <const.h>
#include <assert.h>
#include <lib/stdlib.h>
#include <i386/ioports.h>
#include <driver/vga/vga.h>
#include <driver/vga/text.h>
#include <driver/bochs/bochs.h>

static struct VgaTextDriver VgaTextInfo;

/* 
 * Cette fonction permet d'initialiser le pilote console d'HaoudOS. POUR L'INSTANT,
 * seul le mode graphique VGA 0x03 texte est pris en compte. Dans le futur, HaoudOS
 * pourrai gérer différents mode vidéo mais s'il l'on essage d'utiliser ses fonctions
 * ci-dessous avec un mode vidéo érroné, le rendu sera bien évidemment incorrect.
 */
void VgaTextSetup(void)
{
	VgaTextInfo.real_video_start = 0xB8000;						// Point de départ réel de la mémoire vidéo
	VgaTextInfo.video_start = 0xB8000;							// Début de la console en mémoire vidéo
	VgaTextInfo.video_end = 0xC0000;							// Fin de la mémoire vidéo
	VgaTextInfo.video_len = VgaTextInfo.video_end - VgaTextInfo.video_start;

	VgaTextInfo.page_len = 0xFA0;								// Taille de la console
	VgaTextInfo.x = 0;											// On commence a gauche de l'écran
	VgaTextInfo.y = 3;											// On réserve 3 lignes d'affichage au bootloader
	VgaTextInfo.attrib = 0x07;									// Couleur par défaut d'HaoudOS (caractère gris clair sur fond noir)
	
	VgaTextInfo.max_x = 80;
	VgaTextInfo.max_y = 25;	
	// TODO: implémenter d'autres variable pour gérer plusieurs modes vidéos (80 x 50 ...)

	VgaTextInfo.initialized = TRUE;
	BochsPrintf("[DEBUG]: VGA Text Mode Driver initialized\n");
}

/*
 * Cette fonction efface la page graphique représentée à l'écran et place le
 * curseur et les attributs x et y à zéro
 */
void ClearScreen(void)
{
	unsigned char *video_ptr = (unsigned char *) (VgaTextInfo.video_start);

	for (unsigned int i = 0; i < VgaTextInfo.page_len; i += 2)
	{
		video_ptr[i] = 0;
		video_ptr[i + 1] = VgaTextInfo.attrib;
	}

	VgaTextInfo.x = 0;
	VgaTextInfo.y = 0;

	UpdateCursor();
}


/* 
 * Cette fonction, très simple, permet de scroller l'écran graphique
 * d'une ligne. POUR L'INSTANT, seul une page graphique est supportée,
 * la ligne la plus haute de la console est donc perdue.
 */
void Scrollup(void)
{
	unsigned char *video_ptr = (unsigned char *) (VgaTextInfo.video_start + VgaTextInfo.page_len - VgaTextInfo.max_x * 2);

	memcpy((char *)VgaTextInfo.video_start,
		   (char *)VgaTextInfo.video_start + VgaTextInfo.max_x * 2, 
		           VgaTextInfo.page_len - (VgaTextInfo.page_len / VgaTextInfo.max_y));

	for (unsigned int i = 0; i < VgaTextInfo.max_x * 2; i += 2)
	{
		video_ptr[i] = 0;
		video_ptr[i + 1] = VgaTextInfo.attrib;
	}


	if (VgaTextInfo.y)
		VgaTextInfo.y--;

	UpdateCursor();
}

/*
 * Cette fonction permet d'affiche UN caractère à l'écran et s'occupe de tout
 * (ou presque). En effet, elle gère quelques caractères spéciaux, le scrolling,
 * le retour à la ligne...
 */
void Putc(const char c)
{
	unsigned char *video = NULL;			// On initialisr toujours les pointeurs
	char to_print = cp1252_to_cp437(c);		// Pour afficher les accents dans le bon encodage

	switch (to_print)
	{
		case '\n':							// Retour à la ligne (ligne suivante)
			VgaTextInfo.x = 0;
			VgaTextInfo.y++;
		break;

	
		case '\b':							// Recule d'un caractère (backspace)
			if (VgaTextInfo.x)
				VgaTextInfo.x--;
			else
			{
				if (VgaTextInfo.y)
				{
					VgaTextInfo.y--;
					VgaTextInfo.x = VgaTextInfo.max_x - 1;
				}
			}
			break;

		case '\r':							// Retour chariot (au début de la ligne)
			VgaTextInfo.x = 0;
			break;

		case '\t':							// Tabulation

			// Appele Putc afin d'effacer les caractères qui vont être écrasé par la tabulation
			for(int i = (4 - (VgaTextInfo.x % 4)); i > 0; i--)
				Putc(' ');
			break;

		case '\a':							// Son de cloche (pas encore supporté)
			break;

		default:							// Caractère "normal"
			video = (unsigned char *) (VgaTextInfo.video_start + VgaTextInfo.x * 2 + VgaTextInfo.y * (VgaTextInfo.max_x * 2));
			*video = to_print;
			*(video + 1) = VgaTextInfo.attrib;

			VgaTextInfo.x++;
			break;
	}

	// Si on dépasse la ligne
	if (VgaTextInfo.x >= VgaTextInfo.max_x)
	{
		// On va à la ligne suivante
		VgaTextInfo.x = 0;
		VgaTextInfo.y++;
	}

	// Si nous somme en bas de la console
	if (VgaTextInfo.y >= VgaTextInfo.max_y)	
		Scrollup();					// On scrolle d'un ligne

	UpdateCursor();					// On actualise la position du curseur
}

/*
 * Cette fonction affiche une chaîne de caractère à l'écran grâce
 * à la fonction Putc
 */
void Print(const char *s)
{
	while (*s)
	{
		Putc(*s);
		s++;
	}
}


/*
 * Cette fonction permet de déplacer la position courante du prochain
 * caractère affiché dans la console. La position est changé si la coordonnée
 * n'est pas négative (utilisé pour ne changer que x, par exemple) et que
 * la coordonnée n'est pas plus grande que le maximum autorisé
 */
void GoTo(int x, int y)
{
	if (x >= 0 && x < (int)VgaTextInfo.max_x)
		VgaTextInfo.x = x;

	if (y >= 0 && y < (int)VgaTextInfo.max_y)
		VgaTextInfo.y = y;

	UpdateCursor();
}

/*
 * Cette fonction permet d'actualiser le curseur géré matériellement
 * à la position courant (x et y de la structure VgaTextInfo)
 */
void UpdateCursor(void)
{
	uint16_t cursorPos = (uint16_t) ((VgaTextInfo.y * VgaTextInfo.max_x) + VgaTextInfo.x);
	cursorPos += (VgaTextInfo.video_start - VgaTextInfo.real_video_start) / 2;

	AssertWarning(cursorPos < ((VgaTextInfo.max_y * VgaTextInfo.max_x) + (VgaTextInfo.video_start - VgaTextInfo.real_video_start) / 2));

	// Envoie des 8 premiers octets
	outb(VGA_CMD_PORT, VGA_CMD_LOW_INDEX);
	outb(VGA_DATA_PORT, (uint8_t) (cursorPos & 0xFF));

	// Envoie des 8 octets suivant au registre VGA
	outb(VGA_CMD_PORT, VGA_CMD_HIGH_INDEX);
	outb(VGA_DATA_PORT, (uint8_t) ((cursorPos >> 8) & 0xFF));
}

/*
 * Cette fonction affiche un texte avec la couleur de caractère et d'arrière plan
 * passées en paramètre mais ne modifie pas le couleur des caractères précedemment
 * affichés ni ceux qui le seront dans le futur
 */
void PrintColor(const char *s, enum VGA_COLOR4 font_color, enum VGA_COLOR4 back_color)
{
	uint8_t attribSaved = VgaTextInfo.attrib;
	SetTextColor(font_color, back_color);

	Print(s);

	VgaTextInfo.attrib = attribSaved;
}

void ClearLine(enum ClearType type)
{
	unsigned char *start = NULL;
	int len = 0;

	switch(type)
	{
		case WHOLE_LINE:
			start = (unsigned char *) (VgaTextInfo.video_start + VgaTextInfo.y * (VgaTextInfo.max_x * 2));
			len = VgaTextInfo.max_x * 2;
			break;

		case FROM_CURSOR:
			start = (unsigned char *) (VgaTextInfo.video_start + VgaTextInfo.x * 2 + VgaTextInfo.y * (VgaTextInfo.max_x * 2));
			len = (VgaTextInfo.max_x * 2) - (VgaTextInfo.x * 2);
			break;

		case TO_CURSOR:
			start = (unsigned char *) (VgaTextInfo.video_start + VgaTextInfo.x * 2 + VgaTextInfo.y * (VgaTextInfo.max_x * 2));
			len = VgaTextInfo.x * 2;
			break;

		default:
			return;
	}

	for (int i = 0; i < len; i += 2)
	{
		start[i] = 0;
		start[i + 1] = VgaTextInfo.attrib;
	}
}

/*
 * Cette fonction permet d'effacer l'écran (la 1er page graphique) avec la couleur
 * séléctionnée, mais ne modife pas la couleur des caractères suivants
 */
void ClearScreenColor(enum VGA_COLOR4 font_color, enum VGA_COLOR4 back_color)
{
	uint8_t attrib = (uint8_t) ((back_color << 4) | (font_color));

	unsigned char *video_ptr = (unsigned char *) (VgaTextInfo.video_start);

	for (unsigned int i = 0; i < VgaTextInfo.page_len; i += 2)
	{
		video_ptr[i] = 0;
		video_ptr[i + 1] = attrib;
	}

	VgaTextInfo.x = 0;
	VgaTextInfo.y = 0;

	UpdateCursor();
}

/*
 * Cette fonction permet de modifier la couleur des prochains caractères qui apparaitrons
 * après l'appel de cette fonction mais ne modifie pas la couleur des caractères affichés
 * précédemment
 */
void SetTextColor(enum VGA_COLOR4 font_color, enum VGA_COLOR4 back_color)
{
	if (font_color == NO_CHANGE && back_color == NO_CHANGE)
		return;
	else if (font_color == NO_CHANGE)
		VgaTextInfo.attrib = (VgaTextInfo.attrib & 0x0F) | (uint8_t)(back_color << 4);
	else if (back_color == NO_CHANGE)
		VgaTextInfo.attrib = (VgaTextInfo.attrib & 0xF0) | (uint8_t)(font_color);
	else
		VgaTextInfo.attrib = (uint8_t) ((back_color << 4) | (font_color));
}

void SaveConsole(char *buffer)
{
	memcpy(buffer, (void *)VgaTextInfo.video_start, VgaTextInfo.page_len);
}
