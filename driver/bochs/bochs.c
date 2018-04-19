#include <const.h>
#include <lib/stdio.h>
#include <lib/stdarg.h>
#include <i386/ioports.h>
#include <lib/vsnprintf.h>
#include <driver/bochs/bochs.h>

static struct BochsDriver BochsInfo;

void BochsSetup(void)
{
	//TODO: impl�menter une d�t�ction de Bochs
	BochsInfo.in_bochs = TRUE;					// Par d�faut on suppose que nous somme dans Bochs
	BochsInfo.initialized = TRUE;				// Le driver est maintenant initialis�
	BochsInfo.e9_enabled = e9Enabled();			// D�t�cte si le port 0xe9 de BOCHS est actif (utilise pour le debug)

	BochsPrint("\n\n -========+ HaoudOS Bochs Interface (HBI) +========-\n");
	return; 
}

/*
 * Cette fonction affiche un caract�re ASCII sur le port 0xe9 de BOCHS uniquement si ce
 * port est actif
 */
void BochsPutc(const char c)
{
	if (BochsInfo.initialized && BochsInfo.e9_enabled)
		outb(0xe9, c);
}

/*
 * Cette fonction affiche une chaine de caract�re sur le port 0xe9 de BOCHS gr�ce � la
 * fonction BochsPutc()
 */
void BochsPrint(const char *s)
{
	while(*s)
		BochsPutc(*s++);
}

/*
 * Cette fonction d�tecte si le port 0xe9 est bien activ�, mais ne permet pas
 * la d�t�ction de machine virtuelle BOCHS car il est facile de d�sactiver 
 * cette option ou que le port soit op�rationnel sur machine r�elle
 */
bool_t e9Enabled(void)
{
	uint8_t data = inb(0xe9);
	if (data == 0xe9)
		return TRUE;
	else
		return FALSE;
}

/* 
 * Cette fonction permet d'afficher un cha�ne de caract�re (256 car. max) format�
 * de telle sorte de vsnprintf puissque la comprendre et la modifier en cons�quence
 * Elle permet donc d'afficher des nombres, des pointeurs...
 */
void BochsPrintf(const char *format, ...)
{
	char PrintBuffer[256];
	va_list arg;

	va_start(arg, format);
	vsnprintf(PrintBuffer, 256, format, arg);
	va_end(arg);

	BochsPrint(PrintBuffer);
}

/*
 * Cette fonction affiche sur le port de debug de bochs un dump de la m�moire octet
 * par octet commen�ant � l'adresse 'data', et affiche le nombre d'octet sp�cifi� par
 * le param�tre suivant.
 *
 * Le dump est organis� de cette mani�re:
 * 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F | ..........
 * FF 55 56 									   | .UV
 *
 * Si l'octet en affichable en ASCII pur, alors il est affich�, sinon il est repr�sent�
 * par un point (NOTE: Le point est aussi affich� comme un point...)
 */
void BochsDump(const char *data, const unsigned int lenght)
{
	unsigned int i, j;
	for (i = 0; i < lenght; i++)
	{
		BochsPrintf("%02X ", data[i]);

		if ( (i % 16) == 15 || i == lenght - 1)
		{
			for (j = 0; j < 15 - (i % 16); j++)
				BochsPrint("   ");

			BochsPrint(" | ");
			for (j = (i - (i % 16)); j <= i; j++)
			{
				if ((data[j] > 31) && (data[j] < 127))
					BochsPutc(data[j]);
				else
					BochsPutc('.');
			}
			BochsPrint("\n");
		}
	}
}