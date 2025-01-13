	.even
	.globl _memset
_memset:
	mov r3, -(sp)
	mov r4, -(sp)
	mov r2, r3
	beq done
	bit r1, $1
	beq even
	movb r0, (r1)+
	dec r2
even:
	asr r3
	beq done
	mov r0, r4
	swab r4
	bis r4, r0
word_loop:
	mov r0, (r1)+
	sob r3, word_loop
	bit r1, $1
	beq done
	movb r0, (r1)+
done:
	mov (sp)+, r4
	mov (sp)+, r3
	rts pc
