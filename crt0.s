	.text
	.globl _main, ___main, _start, ___stack_top, _bk_11m
_start:
	mov pc, sp
	sub $4, sp
	cmpb	$0200, @$0177717	/ 0010 или 11М?
	beq	1f
	movb	$0100, @$0177663	/ палитра 0, экран - стр. 5, кадровое прерывание запрещено
	mov	$1, _bk_11m
1:
	mov $01330, r0
	mov r0, @$0177664
	mov	$_start,@$04
	jsr	pc, _main
	halt

___main:
	rts	pc

	.data
_bk_11m:	.word 0		/ не 0 на БК 0011М
