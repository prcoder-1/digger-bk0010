	.text
	.globl _main, ___main, _start, ___stack_top, _bk_11m, _ay_azbk
_start:
	mov pc, sp
	sub $4, sp
	cmpb	$0200, @$0177717	/ 0010 или 11М?
	beq	1f
	movb	$0100, @$0177663	/ палитра 0, экран - стр. 5, кадровое прерывание запрещено
	inc	_bk_11m
1:
	mov	$2f, @$04		/ AZБК: регистры AY 0177172/0177173 (нет - ошибка шины)
	clrb	@$0177172
	mov	@$0177550, r0		/ ГСЧ AZБК: каждое чтение - новое число,
	cmp	@$0177550, r0		/ ОЗУ или ПЗУ по этим адресам вернут одно и то же
	beq	3f
	inc	_ay_azbk
	br	3f
2:	cmp	(sp)+, (sp)+		/ снять PC и PSW ошибки шины
3:
	mov	$01330, @$0177664
	mov	$_start,@$04
	jsr	pc, _main
	halt

___main:
	rts	pc

	.data
_bk_11m:	.word 0		/ не 0 на БК 0011М
_ay_azbk:	.word 0		/ не 0 - AZБК: AY в 0177172/0177173, прямой код
