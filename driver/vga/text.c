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
 * pourrai g�rer diff�rents mode vid�o mais s'il l'on essage d'utiliser ses fonctions
 * ci-dessous avec un mode vid�o �rron�, le rendu sera bien �videmment incorrect.
 */
void VgaTextSetup(void)
{
	VgaTextInfo.real_video_start = 0xB8000;						// Point de d�part r�el de la m�moire vid�o
	VgaTextInfo.video_start = 0xB8000;							// D�but de la console en m�moire vid�o
	VgaTextInfo.video_end = 0xC0000;							// Fin de la m�moire vid�o
	VgaTextInfo.video_len = VgaTextInfo.video_end - VgaTextInfo.video_start;

	VgaTextInfo.page_len = 0xFA0;								// Taille de la console
	VgaTextInfo.x = 0;											// On commence a gauche de l'�cran
	VgaTextInfo.y = 3;											// On r�serve 3 lignes d'affichage au bootloader
	VgaTextInfo.attrib = 0x07;									// Couleur par d�faut d'HaoudOS (caract�re gris clair sur fond noir)
	
	VgaTextInfo.max_x = 80;
	VgaTextInfo.max_y = 25;	
	// TODO: impl�menter d'autres variable pour g�rer plusieurs modes vid�os (80 x 50 ...)

	VgaTextInfo.initialized = TRUE;
	BochsPrintf("[DEBUG]: VGA Text Mode Driver initialized\n");
}

/*
 * Cette fonction efface la page graphique repr�sent�e � l'�cran et place le
 * curseur et les attributs x et y � z�ro
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
 * Cette fonction, tr�s simple, permet de scroller l'�cran graphique
 * d'une ligne. POUR L'INSTANT, seul une page graphique est support�e,
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
 * Cette fonction permet d'affiche UN caract�re � l'�cran et s'occupe de tout
 * (ou presque). En effet, elle g�re quelques caract�res sp�ciaux, le scrolling,
 * le retour � la ligne...
 */
void Putc(const char c)
{
	unsigned char *video = NULL;			// On initialisr toujours les pointeurs
	char to_print = cp1252_to_cp437(c);		// Pour afficher les accents dans le bon encodage

	switch (to_print)
	{
		case '\n':							// Retour � la ligne (ligne suivante)
			VgaTextInfo.x = 0;
			VgaTextInfo.y++;
		break;

	
		case '\b':							// Recule d'un caract�re (backspace)
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

		case '\r':							// Retour chariot (au d�but de la ligne)
			VgaTextInfo.x = 0;
			break;

		case '\t':							// Tabulation

			// Appele Putc afin d'effacer les caract�res qui vont �tre �cras� par la tabulation
			for(int i = (4 - (VgaTextInfo.x % 4)); i > 0; i--)
				Putc(' ');
			break;

		case '\a':							// Son de cloche (pas encore support�)
			break;

		default:							// Caract�re "normal"
			video = (unsigned char *) (VgaTextInfo.video_start + VgaTextInfo.x * 2 + VgaTextInfo.y * (VgaTextInfo.max_x * 2));
			*video = to_print;
			*(video + 1) = VgaTextInfo.attrib;

			VgaTextInfo.x++;
			break;
	}

	// Si on d�passe la ligne
	if (VgaTextInfo.x >= VgaTextInfo.max_x)
	{
		// On va � la ligne suivante
		VgaTextInfo.x = 0;
		VgaTextInfo.y++;
	}

	// Si nous somme en bas de la console
	if (VgaTextInfo.y >= VgaTextInfo.max_y)	
		Scrollup();					// On scrolle d'un ligne

	UpdateCursor();					// On actualise la position du curseur
}

/*
 * Cette fonction affiche une cha�ne de caract�re � l'�cran gr�ce
 * � la fonction Putc
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
 * Cette fonction permet de d�placer la position courante du prochain
 * caract�re affich� dans la console. La position est chang� si la coordonn�e
 * n'est pas n�gative (utilis� pour ne changer que x, par exemple) et que
 * la coordonn�e n'est pas plus grande que le maximum autoris�
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
 * Cette fonction permet d'actualiser le curseur g�r� mat�riellement
 * � la position courant (x et y de la structure VgaTextInfo)
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
 * Cette fonction affiche un texte avec la couleur de caract�re et d'arri�re plan
 * pass�es en param�tre mais ne modifie pas le couleur des caract�res pr�cedemment
 * affich�s ni ceux qui le seront dans le futur
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
 * Cette fonction permet d'effacer l'�cran (la 1er page graphique) avec la couleur
 * s�l�ctionn�e, mais ne modife pas la couleur des caract�res suivants
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
 * Cette fonction permet de modifier la couleur des prochains caract�res qui apparaitrons
 * apr�s l'appel de cette fonction mais ne modifie pas la couleur des caract�res affich�s
 * pr�c�demment
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
