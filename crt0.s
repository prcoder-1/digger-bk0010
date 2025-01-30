	.text
	.globl _main, ___main, _start, ___stack_top
_start:
	mov pc, sp
	sub $4, sp
	mov $01330, r0
	mov r0, @$0177664
	mov	$_start,@$04
	jsr	pc, _main
	halt

___main:
	rts	pc
