	.text
	.globl _main, ___main, _start

_start:
	cmp	sp, pc
	blo	.+4
	mov	pc, sp
	jsr	pc, _main
	halt

___main:
	rts	pc
