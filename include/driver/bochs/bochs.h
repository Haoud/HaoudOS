#pragma once
#include <types.h>

struct BochsDriver
{
	bool_t in_bochs;
	bool_t initialized;
	bool_t e9_enabled;
};

void BochsSetup(void);
bool_t e9Enabled(void);
void BochsPutc(const char c);
void BochsPrint(const char *s);
void BochsPrintf(const char *format, ...);
void BochsDump(const char *data, const unsigned int lenght);

