;
; This file was created on Tue Apr 17 2018
; Copyright 2018 Romain CADILHAC
;
; This file is a part of HaoudOS.
;
; HaoudOS is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; HaoudOS is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
; GNU General Public License for more details.
; You should have received a copy of the GNU General Public License
; along with HaoudOS. If not, see <http://www.gnu.org/licenses/>.
;
[BITS 32]
extern do_signal

%macro HANDLER_SAVE_REGS 0
	pushad
	push ds
	push es
	push fs
	push gs
	push ss
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
%endmacro

%macro HANDLER_RESTORE_REGS 0
	pop ss
	pop gs
	pop fs
	pop es
    pop ds
	popad
%endmacro

%macro wrap_int 1
global asm_%1
extern %1

asm_%1:
    HANDLER_SAVE_REGS
    push esp
    call %1
    add esp, 4
	jmp ret_from_intr
%endmacro

%macro wrap_ex 1
global asm_%1
extern %1

asm_%1:
    HANDLER_SAVE_REGS
    push esp
    call %1
    add esp, 4				; Enlève le code d'erreur
	jmp ret_from_ex
%endmacro

ret_from_ex:
	push esp
	call do_signal
	add esp, 4
    HANDLER_RESTORE_REGS
	add esp, 4
    iretd

ret_from_intr:
	push esp
	call do_signal
	add esp, 4
    HANDLER_RESTORE_REGS
    iretd

ret_from_syscall:
	mov [SyscallReturn], eax
	push esp
	call do_signal
	add esp, 4
    HANDLER_RESTORE_REGS
	mov eax, [SyscallReturn]
    iretd

wrap_int default_handler
wrap_int TimerTick
wrap_ex page_fault
wrap_int Keyboard_IRQ

wrap_int divide_by_zero
wrap_int debug
wrap_int nmi
wrap_int breakpoint
wrap_int overflow
wrap_int bound	
wrap_int invalide_op
wrap_int device_not_avaible			
wrap_int double_fault			
wrap_int coprocessor_error	
wrap_int invalide_tss
wrap_int segment_not_present
wrap_int stack_fault
wrap_int general_protection_fault
wrap_int fpu_exception

; Gestion spéciale pour les appels système
global asm_SyscallCore
extern SyscallCore

SyscallReturn dd 0

asm_SyscallCore:
    HANDLER_SAVE_REGS
    push esp
    call SyscallCore
	add esp, 4
	jmp ret_from_syscall

; Gestion spéciale pour le scheduler
global asm_SchedulerCore
extern SchedulerCore

asm_SchedulerCore:
    pushad
	push ds
	push es
	push fs
	push gs
	push ss

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	push esp
	call SchedulerCore
	add esp, 4

	mov esp, eax					; Changement de pile noyau

	; Enlève le masque du PIC
	mov al, 0x20
	out 0x20, al

	jmp ret_from_intr				
