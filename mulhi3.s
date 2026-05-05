	.text
	.even
	.globl ___mulhi3
___mulhi3:
	mov	r2, -(sp)
	clr	r2
	mov	04(sp), r0
	mov	06(sp), r1
	tst	r0
	beq	2f
1:	clc
	ror	r0
	bcc	3f
	add	r1, r2
3:	asl	r1
	tst	r0
	bne	1b
2:	mov	r2, r0
	mov	(sp)+, r2
	rts	pc
