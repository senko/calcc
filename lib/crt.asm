;
; Basic system interface for process startup, shutdown and minimal I/O
;
;	Authors:
;		Matija Cizmek <mcizmek@fly.srk.fer.hr>
;		Senko Rasic <senko@senko.net>
;
;	Resources:
;	  * Reference to Linux 2.{2,4} System Calls for Assembly Level Access
;	  * uClibc (http://www.uclibc.org/) source code
;

bits 32

section text

%include "syscall.inc"

extern main

; standard C/Unix program start trampoline
; sets up arguments for 'main' and calls it; upon main's return calls 'exit'

global _start:function
_start:
	pop	ecx		; argc -> %ecx, %edx
	mov	edx, ecx
	mov	eax, esp	; argv -> %eax, %ebx
	mov	ebx, esp

	; env = argv + (argc * 4) + 4
	lea	eax, [ebx + edx * 4 + 4]

	; set up an invalid callers stack
	xor	ebp, ebp
	push	ebp
	push	ebp
	push	ebp
	mov	ebp, esp

	; initialize env, argv and argc
	push	eax		; env
	push	ebx		; argv
	push	ecx		; argc

	call	main

	; 'main' should return exit code via %eax
	push	eax
	call	exit


; void exit(int32 status)
;	terminate the current process
; args:
;	status: status code
; returns:
;	doesn't return

global exit:function
exit:
	mov	ebx, [esp + 4]
	mov	eax, 1
	int	0x80


; int write(int32 fd, void *buf, int32 count)
;	write to a file descriptor
; args:
;	fd: file descriptor to write to
;	buf: pointer to buffer
;	count: buffer size (in bytes)
; returns:
;	number of bytes written or error code

global write:function
write:
	push    ebp
	mov     ebp, esp

	mov     ebx, [ebp + 8]
	mov     ecx, [ebp + 12]
	mov     edx, [ebp + 16]
	mov     eax, sys_write
	int     0x80

	pop     ebp
	ret

; int read(int32 fd, void *buf, int32 count)
;	read from a file descriptor
; args:
;	fd: file descriptor to read from
;	buf: pointer to buffer
;	count: buffer size (in bytes)
; returns:
;	number of bytes read or error code

global read:function
read:
	push	ebp
	mov	ebp, esp

	mov	ebx, [ebp + 8]
	mov	ecx, [ebp + 12]
	mov	edx, [ebp + 16]
	mov	eax, sys_read
	int	0x80

	pop	ebp
	ret

; int getch(int32 fd)
;	read one character from a file descriptor
; args:
;	fd: file descriptor to read from
; returns:
;	character read or error code

global getch:function
getch:
	push	ebp
	mov	ebp, esp

	mov	ebx, [ebp + 8]
	mov	ecx, ebp 
	dec	ecx
	mov	edx, 1
	mov	eax, sys_read
	int	0x80 

	cmp	eax, byte 1
	jz	.Lf0
	mov	eax, -1
	jmp	.Lfe
.Lf0:
	xor	eax, eax
	mov	al, [ebp - 1]
.Lfe:
	pop	ebp
	ret

; int putch(long fd, byte char)
;	write one character to file descriptor
; args:
;	fd: file descriptor to write to
;	char: character to write
; returns:
;	0 or error code

global putch:function
putch:
	push	ebp
	mov	ebp, esp

	mov	ebx, [ebp + 8]
	mov	ecx, ebp 
	add	ecx, 12
	mov	edx, 1
	mov	eax, sys_write
	int	0x80 

	cmp	eax, 1
	jz	.Lf0
	mov	eax, -1
	jmp	.Lfe
.Lf0:
	mov	eax, 0
.Lfe:
	pop	ebp
	ret

