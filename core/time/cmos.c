#include <i386/ioports.h>
#include <core/time/cmos.h>

hret_t cmos_init(void)
{
	return RET_OK;
}

uint8_t cmos_read(uint8_t reg)
{
	outb(CMOS_ADDRESS, reg);		// Indique le registre que nous voulons lire
	return inb(CMOS_DATA);			// Lit le registre désiré
}

void cmos_write(uint8_t reg, uint8_t value)
{
	outb(CMOS_ADDRESS, reg);		// Indique le registre que nous voulons modifier
	outb(CMOS_DATA, value);			// Ecrit le registre qu'il faut modifier
}