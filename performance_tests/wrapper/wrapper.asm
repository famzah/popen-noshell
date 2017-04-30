; http://linux-assembly.vger.kernel.narkive.com/0ycrIQiS/example-of-execve
;
; execve(2) syscall asm example
; nasm syntax

SECTION .text

global _start
_start:
	mov ebx, array
	lea ecx, [array+8]
	lea edx, [array+12]
	mov eax, 0xb
	int 0x80

SECTION .data

	array db './tiny2', 0
	dd array
	dd 0
