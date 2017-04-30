; http://0xax.blogspot.bg/2014/08/say-hello-to-x64-assembly-part-1.html

section .data
	msg	db	"hello, world!",0xa
	len	equ	$ - msg                 ; length of the string

section .text
	global _start
_start:
	mov	rax, 1
	mov	rdi, 1
	mov	rsi, msg
	mov	rdx, len
	syscall

	mov	rax, 60
	mov	rdi, 0
	syscall
