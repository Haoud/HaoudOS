#pragma once
#include <types.h>

struct VgaTextDriver
{
	ptr_t real_video_start;	//
	ptr_t video_start;		// En général 0xB8000
	ptr_t video_end;		// En général 0xC0000

	size_t video_len;		// Simple soustraction entre video_end et video_start
	size_t page_len;		// Dépend du mode vidéo

	uint32_t x;
	uint32_t y;
	uint32_t max_x;
	uint32_t max_y;
	uint8_t attrib;			// Couleur du prochain caractère

	bool_t initialized;		// Parle de lui même...
};

/* Couleur VGA sur 4 bit (mode texte 0x03) */
enum VGA_COLOR4
{
	BLACK	= 0,
	BLUE	= 1,
	GREEN	= 2,
	CYAN	= 3,
	RED		= 4,
	MAGENTA	= 5,
	BROWN	= 6,
	LIGHT_GREY	= 7,
	DARK_GREY	= 8,
	LIGHT_BLUE	= 9,
	LIGHT_GREEN	= 10,
	LIGHT_CYAN	= 11,
	LIGHT_RED	= 12,
	LIGHT_MAGENTA	= 13,
	YELLOW	= 14,
	WHITE	= 15,
	NO_CHANGE = 16				// Ne pas changer la couleur (ne fait pas parti des couleurs VGA)
};

enum ClearType
{
	WHOLE_LINE,			// Toute la ligne courant
	FROM_CURSOR,		// Du curseur jusqu'à la fin de la ligne
	TO_CURSOR,			// Du début de la ligne jusqu'au curseur
};

void Scrollup(void);
void ClearScreen(void);
void VgaTextSetup(void);
void Putc(const char c);
void UpdateCursor(void);
void GoTo(int x, int y);
void Print(const char *s);
void SaveConsole(char *buffer);
void ClearLine(enum ClearType type);
void SetTextColor(enum VGA_COLOR4 font_color, enum VGA_COLOR4 back_color);
void ClearScreenColor(enum VGA_COLOR4 font_color, enum VGA_COLOR4 back_color);
void PrintColor(const char *s, enum VGA_COLOR4 font_color, enum VGA_COLOR4 back_color);
