#pragma once
#include <types.h>

#define CMOS_ADDRESS				0x70
#define CMOS_DATA					0x71

#define CMOS_REG_SECONDS			0x00
#define CMOS_REG_SECONDS_ALARM		0x01
#define CMOS_REG_MINUTES			0x02
#define CMOS_REG_MINUTES_ALARM		0x03
#define CMOS_REG_HOURS				0x04
#define CMOS_REG_HOURS_ALARM		0x05
#define CMOS_REG_WEEK_DAY			0x06
#define CMOS_REG_DATE_DAY			0x07
#define CMOS_REG_DATE_MONTH			0x08
#define CMOS_REG_DATE_YEAR			0x09
#define CMOS_REG_STAT_A				0x0A
	#define STAT_A_UPDATE_IN_PROGRESS				0x40
#define CMOS_REG_STAT_B				0x0B
#define CMOS_REG_STAT_C				0x0C
#define CMOS_REG_STAT_D				0x0D
#define CMOS_REG_DIAGNOSTIC			0x0E
#define CMOS_REG_CENTURY			0x32			// N'est pas présent sur de vieux CMOS
// D'autres registres existent mais ne sont pas indispensable
// TODO: completer la liste des options pour les registres de stat

#define bcd_to_decimal(bcd)	((bcd >> 4) * 10 + (bcd & 0x0F))

hret_t cmos_init(void);
uint8_t cmos_read(uint8_t reg);
void cmos_write(uint8_t reg, uint8_t value);
