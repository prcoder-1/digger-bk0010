	.text

	.even
	.globl _udiv
_udiv:
	mov r5, -(sp)
	mov sp, r5
	add $-016, sp
	mov r2, -(sp)
	mov r3, -(sp)
	mov r4, -(sp)
	clr -014(r5)
	mov 04(r5), -016(r5)
	mov 04(r5), r4
	mov r4, r3
	clr r2
	mov r2,-06(r5)
	mov r3,-04(r5)
	mov 06(r5), r2
	mov r2, r1
	clr r0
	mov r0,-012(r5)
	mov r1,-010(r5)
	clr -02(r5)
	br L_2
L_3:
	mov -012(r5),r0
	mov -010(r5),r1
	add r0, r0
	add r1, r1
	adc r0
	mov r0,-012(r5)
	mov r1,-010(r5)
	inc -02(r5)
L_2:
	cmp -02(r5),$015
	ble L_3
	clr -02(r5)
	br L_4
L_7:
	mov -014(r5), r0
	asl r0
	mov r0, -014(r5)
	cmp -012(r5),-06(r5)
	bhi L_5
	cmp -012(r5),-06(r5)
	bne L_10
	cmp -010(r5),-04(r5)
	bhi L_5
L_10:
	sub -012(r5), -06(r5)
	sub -010(r5), -04(r5)
	sbc -06(r5)
	mov -04(r5), -016(r5)
	bis $01, -014(r5)
L_5:
	mov -012(r5),r0
	mov -010(r5),r1
	clc
	ror r0
	ror r1
	mov r0,-012(r5)
	mov r1,-010(r5)
	inc -02(r5)
L_4:
	cmp -02(r5),$016
	ble L_7
	cmp 010(r5),$01
	bne L_8
	mov -016(r5), r0
	br L_9
L_8:
	mov -014(r5), r0
L_9:
	mov (sp)+, r4
	mov (sp)+, r3
	mov (sp)+, r2
	mov r5, sp
	mov (sp)+, r5
	rts pc
	.even
	.globl _div
_div:
	mov r5, -(sp)
	mov sp, r5
	add $-04, sp
	clr -02(r5)
	clr -04(r5)
	tst 04(r5)
	bge L_12
	neg 04(r5)
	tst 06(r5)
	bge L_13
	neg 06(r5)
	cmp 010(r5),$01
	bne L_16
	mov $01, -02(r5)
	br L_16
L_13:
	mov $01, -02(r5)
	br L_16
L_12:
	tst 06(r5)
	bge L_16
	neg 06(r5)
	tst 010(r5)
	bne L_16
	mov $01, -02(r5)
L_16:
	tst -02(r5)
	beq L_18
	mov 06(r5), r1
	mov 04(r5), r0
	mov 010(r5), -(sp)
	mov r1, -(sp)
	mov r0, -(sp)
	jsr pc, _udiv
	add $06, sp
	neg r0
	br L_19
L_18:
	mov 06(r5), r1
	mov 04(r5), r0
	mov 010(r5), -(sp)
	mov r1, -(sp)
	mov r0, -(sp)
	jsr pc, _udiv
	add $06, sp
L_19:
	mov r5, sp
	mov (sp)+, r5
	rts pc
	.even
	.globl ___udivhi3
___udivhi3:
	mov r5, -(sp)
	mov sp, r5
	clr -(sp)
	mov 06(r5), -(sp)
	mov 04(r5), -(sp)
	jsr pc, _udiv
	add $06, sp
	mov (sp)+, r5
	rts pc
	.even
	.globl ___umodhi3
___umodhi3:
	mov r5, -(sp)
	mov sp, r5
	mov $01, -(sp)
	mov 06(r5), -(sp)
	mov 04(r5), -(sp)
	jsr pc, _udiv
	add $06, sp
	mov (sp)+, r5
	rts pc
	.even
	.globl ___divhi3
___divhi3:
	mov r5, -(sp)
	mov sp, r5
	clr -(sp)
	mov 06(r5), -(sp)
	mov 04(r5), -(sp)
	jsr pc, _div
	add $06, sp
	mov (sp)+, r5
	rts pc
	.even
	.globl ___modhi3
___modhi3:
	mov r5, -(sp)
	mov sp, r5
	mov $01, -(sp)
	mov 06(r5), -(sp)
	mov 04(r5), -(sp)
	jsr pc, _div
	add $06, sp
	mov (sp)+, r5
	rts pc
	.even
	.globl ___mulhi3
___mulhi3:
	mov r5, -(sp)
	mov sp, r5
	add $-014, sp
	mov $017, -04(r5)
	clr -014(r5)
	clr -012(r5)
	tst 04(r5)
	bge L_30
	neg 04(r5)
	tst 06(r5)
	bge L_31
	neg 06(r5)
	clr -02(r5)
	br L_33
L_31:
	mov $01, -02(r5)
	br L_33
L_30:
	tst 06(r5)
	bge L_34
	neg 06(r5)
	mov $01, -02(r5)
	br L_33
L_34:
	clr -02(r5)
L_33:
	mov 04(r5), -06(r5)
	tst -010(r5)
	bge LSXT
	mov $-1,-010(r5)
	br L_35
LSXT:
	clr -010(r5)
	br L_35
L_37:
	mov 06(r5), r0
	bic $-02, r0
	tst r0
	beq L_36
	add -010(r5), -014(r5)
	add -06(r5), -012(r5)
	adc -014(r5)
L_36:
	mov 06(r5), r0
	asr r0
	mov r0, 06(r5)
	mov -010(r5),r0
	mov -06(r5),r1
	add r0, r0
	add r1, r1
	adc r0
	mov r0,-010(r5)
	mov r1,-06(r5)
	dec -04(r5)
L_35:
	tst -04(r5)
	bgt L_37
	tst -02(r5)
	beq L_38
	mov -012(r5), r0
	bis $-0100000, r0
	br L_39
L_38:
	mov -012(r5), r0
L_39:
	mov r5, sp
	mov (sp)+, r5
	rts pc
