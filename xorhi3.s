	.text
	.even
	.globl ___xorhi3
___xorhi3:
	mov	02(sp), r0
	mov	04(sp), r1
	xor	r1, r0
	rts	pc
