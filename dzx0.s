/ Распаковщик ZX0 v2.2 (формат Einar Saukas & Urusergi).
/ PDP11-версия reddie, 02-may-2024 (upd), 92 байта — перевод в синтаксис
/ GNU as из /home/prcoder/БК0010/lz0/dzx0_pdp11_upd.asm.
/
/ dzx0:  r0 — сжатый поток, r1 — приёмник;
/        на выходе r0/r1 — за концом источника/приёмника;
/        портит все регистры, кроме r5.
/
/ _zx0_decompress(src, dst) — обвязка под C ABI (аргументы на стеке,
/ r2-r4 сохраняются).
	.text
	.even
	.globl	_zx0_decompress
_zx0_decompress:
	mov	r2, -(sp)
	mov	r3, -(sp)
	mov	r4, -(sp)
	mov	010(sp), r0	/ src
	mov	012(sp), r1	/ dst
	jsr	pc, dzx0
	mov	(sp)+, r4
	mov	(sp)+, r3
	mov	(sp)+, r2
	rts	pc

dzx0:
	clr	r2
	movb	$0200, r3	/ N=1: следующий sxt даст last_offset = -1
	sxt	-(sp)

dzx0_literals:
	jsr	pc, dzx0_elias
1:	movb	(r0)+, (r1)+
	sob	r2, 1b
	aslb	r3
	bcs	dzx0_new_offset
	jsr	pc, dzx0_elias

dzx0_copy:
	mov	r1, r4
	add	(sp), r4	/ (sp) — отрицательное смещение истории
1:	movb	(r4)+, (r1)+
	sob	r2, 1b
	aslb	r3
	bcc	dzx0_literals

dzx0_new_offset:
	tst	(sp)+
	mov	$-2, r2
	jsr	pc, dzx0_elias_loop
	incb	r2
	bne	1f
dzx0_exit:			/ он же выход из dzx0_elias по bcs
	rts	pc

1:	swab	r2
	mov	r2, -(sp)
	movb	(r0)+, (sp)
	asr	(sp)		/ C = бит обратного хода гаммы
	mov	$1, r2
	bcs	2f
	jsr	pc, dzx0_elias_backtrack
2:	inc	r2
	br	dzx0_copy

dzx0_elias:
	incb	r2

dzx0_elias_loop:
	aslb	r3
	bne	dzx0_elias_skip
	movb	(r0)+, r3
	rolb	r3

dzx0_elias_skip:
	bcs	dzx0_exit

dzx0_elias_backtrack:
	aslb	r3
	rol	r2
	br	dzx0_elias_loop
