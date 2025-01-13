#include "sound.h"

void sound(uint16_t period, uint16_t durance)
{
    asm volatile (
        "mov %[period], r1\n\t"  // r1 = period
        "mov %[durance], r2\n\t" // r2 = durance
        "mov $0100, r3\n\t"
        "clr r0\n\t"
".l1_%=:\n\t"
        "mov r1, r4\n"
        "mov r0, @$-062\n\t"
".l2_%=:\n\t"
        "sob r4, .l2_%=\n\t"
        "xor r3, r0\n\t"
        "sob r2, .l1_%=\n\t"
        :
        : [period]"g"(period), [durance]"g"(durance)
        : "r0", "r1", "r2", "r3", "r4", "cc", "memory"
    );
}

void sound_vibrato(uint16_t period, uint16_t durance)
{
    asm volatile (
        "mov %[period], r1\n\t"  // r1 = period
        "mov %[durance], r2\n\t" // r2 = durance
        "mov $0100, r3\n\t"
        "clr r0\n\t"
".l1_%=:\n\t"
        "mov r1, r4\n"
        "mov r0, @$-062\n\t"
        "bit $0100, r0\n\t"
        "beq .l2_%=\n\t"
        "asr r4\n\t"
        "asr r4\n\t"
        "asr r4\n\t"
        "br .l3_%=\n\t"
".l2_%=:\n\t"
        "asl r4\n\t"
        "asl r4\n\t"
        "asl r4\n\t"
".l3_%=:\n\t"
        "sob r4, .l3_%=\n\t"
        "xor r3, r0\n\t"
        "sob r2, .l1_%=\n\t"
        :
        : [period]"g"(period), [durance]"g"(durance)
        : "r0", "r1", "r2", "r3", "r4", "cc", "memory"
    );
}
