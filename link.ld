OUTPUT_FORMAT("elf32-i386")
ENTRY(_entry)
OUTPUT_ARCH("i386")

SECTIONS
{
    . = 0x100000;

    entry :
    {   
        *(entry*)
    }

    .text :
    {   
        *(.text*)
    }

    .data :
    {   
        *(.data*)
        *(.bss)
        *(COMMON)
    }

	.rodata :
	{
		*(.rodata*)
	    *(.eh_frame*)
	}
}
