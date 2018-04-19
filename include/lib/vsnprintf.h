#pragma once

#include <types.h>
#include <lib/stdarg.h>

#define VSNPRINT_ZEROPAD	0x01		// Padding avec des zéros
#define VSNPRINT_SIGN		0x02		// Le résultat est t-il signé ou non ?
#define VSNPRINT_PLUS		0x04		// Ajoute un plus si le résultat est positif
#define VSNPRINT_SPACE		0x08		// Ajoute un espace à la place du plus
#define VSNPRINT_LARGE		0x10		// Par exemple, 0xdeadbeef devient 0xDEADBEEF

int snprintf(char *buf, const size_t n, const char *format, ...);
int vsnprintf(char *buf, const size_t len, const char *format, va_list arg);
char *number(char *str, int num, int base, int size, unsigned int *max_size, flags_t options);