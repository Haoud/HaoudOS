#pragma once
#include <types.h>

#define iowait(){					\
	for(int i = 0; i < 10; i++)		\
		asm volatile("nop");		\
}

#define outb(port,data) 			\
	asm volatile ("outb dx, al" :: "d" (port), "a" (data) : "memory")

#define outbp(port,data) 			\
	asm volatile ("outb dx, al" :: "d" (port), "a" (data) : "memory");	\
	iowait()

#define outw(port,data) 			\
	asm volatile ("outw dx, ax" :: "d" (port), "a" (data) : "memory")

#define inb(port) ({   				\
	unsigned char _v;      			\
	asm volatile ("inb al, dx" : "=a" (_v) : "d" (port) : "memory"); 	\
	_v;     						\
})

#define inw(port) ({				\
	uint16_t _v;								\
	asm volatile ("inw ax, dx" : "=a" (_v) : "d" (port) : "memory");	\
	_v;								\
})
