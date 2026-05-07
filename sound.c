#include "sound.h"
#include "memory.h"

void sound(uint16_t period, uint16_t durance)
{
    asm volatile (
        "mov %[period], r1\n\t"
        "mov %[durance], r2\n\t"
        "mov $0100, r3\n\t"
        "clr r0\n\t"
".l1_%=:\n\t"
        "mov r1, r4\n"
        "mov r0, @$-062\n\t"
        "xor r3, r0\n\t"
".l2_%=:\n\t"
        "sob r4, .l2_%=\n\t"
        "sob r2, .l1_%=\n\t"
        :
        : [period]"g"(period), [durance]"g"(durance)
        : "r0", "r1", "r2", "r3", "r4", "cc", "memory"
    );
}

void sound_vibrato(uint16_t period, uint16_t durance)
{
    asm volatile (
        "mov %[period], r1\n\t"
        "mov %[durance], r2\n\t"
        "mov $0100, r3\n\t"
        "mov r1, r5\n\t"
        "asr r5\n\t"
        "asr r5\n\t"
        "asr r5\n\t"
        "neg r5\n\t"
        "add r1, r5\n\t"
        "clr r0\n"
".l1_%=:\n\t"
        "mov r1, r4\n\t"
        "mov r0, @$-062\n\t"
        "bit $0100, r0\n\t"
        "beq .l_long_%=\n\t"
        "sub r5, r4\n\t"
        "br .l_inner_%=\n"
".l_long_%=:\n\t"
        "add r5, r4\n"
".l_inner_%=:\n\t"
        "sob r4, .l_inner_%=\n\t"
        "xor r3, r0\n\t"
        "sob r2, .l1_%=\n\t"
        :
        : [period]"g"(period), [durance]"g"(durance)
        : "r0", "r1", "r2", "r3", "r4", "r5", "cc", "memory"
    );
}
