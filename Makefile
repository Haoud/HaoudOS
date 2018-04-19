CC = gcc-7
CCFLAGS = -pipe -ffreestanding -nostdlib -nostdinc -c -g -masm=intel -fno-builtin -fno-stack-protector -m32 -I./include/ -O0 -fpermissive
LIBGCC  = $(shell $(CC) -m32 -print-libgcc-file-name)

AS = nasm
ASFLAGS = -f elf

LD = ld
LDFLAGS = -T link.ld -nostdlib

SRC_ARCH = $(shell find ./arch -type f | grep -E ".c$$")
SRC_CORE := $(shell find ./core -type f | grep -E ".c$$")
SRC_DRIVER := $(shell find ./driver -type f | grep -E ".c$$")
SRC_LIB := $(shell find ./lib -type f | grep -E ".c$$")
SRC_USR := ./usr/init.c

SRC = $(SRC_ARCH) $(SRC_CORE) $(SRC_DRIVER) $(SRC_LIB) $(SRC_USR)
OBJ = $(SRC:.c=.o) arch/i386/interrupt/isr_wrapper.o

TARGET = KERNEL32.SYS

all: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) $^ -o KERNEL32.ELF $(LIBGCC)
	objcopy -O binary KERNEL32.ELF KERNEL32.SYS
	cp $(TARGET) ./../iso/

%.o: %.c 
	$(CC) $(CCFLAGS) $^ -o $@

%.o: %.asm 
	$(AS) $(ASFLAGS) $^ -o $@

clean:
	rm -f $(OBJ) 
