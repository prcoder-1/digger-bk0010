/ Разворот 16-битного слова по горизонтали: тетрады пикселей меняются местами
/ (ABCD EFGH -> DCBA GHFE, где каждая буква - пара бит одного пикселя).
/
/ _mirror_word: вход и выход в r0, портит r1, вызывается через jsr pc.
/
/ Общая для sp_4_15_h_mirror_put (sprites.c) и sp_4_15_hv_mirror_put
/ (sprites_extra.c): обе рисуют отзеркаленный спрайт и раньше несли по
/ собственной дословной копии этих 25 инструкций (~50 байт) - оба модуля
/ попадают в образ игры, так что копия лежала в нём дважды.
	.text
	.even
	.globl	_mirror_word
_mirror_word:
	mov	r0, r1		/ r0 = r1 = ABCD EFGH
	bic	$0x3333, r1	/      r1 = 0B0D 0F0H
	bic	r1, r0		/      r0 = A0C0 E0G0

	asl	r0
	asl	r0		/      r0 = 0A0C 0E0G

	clc
	ror	r1
	ror	r1		/      r1 = B0D0 F0H0

	bis	r1, r0		/      r0 = BADC FEGH
	mov	r0, r1		/      r1 = BADC FEGH

	bic	$0xF0F0, r1	/      r1 = 00DC 00GH
	bic	r1, r0		/      r0 = BA00 FE00

	asl	r1
	asl	r1
	asl	r1
	asl	r1		/      r1 = 00BA 00FE

	clc
	ror	r0
	ror	r0
	ror	r0
	ror	r0		/      r0 = DC00 GH00

	bis	r1, r0		/      r0 = DCBA GHFE
	rts	pc
