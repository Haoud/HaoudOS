#pragma once
#include <usr/libc/stdio.h>

inline static void __attribute__((always_inline)) exit(int code) 
{
	asm("mov eax, %0					\n\
		 mov ebx, %1					\n\
		 int 0x80" :: "i"(SYSCALL_EXIT), "m"(code) :  "memory", "eax", "ebx", "ecx", "edx", "edi", "esi");
}

inline static int __attribute__((always_inline)) fork(void)
{
	int retval = 0;
	asm("mov eax, %1				\n\
		 int 0x80					\n\
		 mov %0, eax" : "=m"(retval) : "i"(SYSCALL_FORK) : "memory", "eax", "ebx", "ecx", "edx", "edi", "esi");

	return retval;
}