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
    // Асимметричный меандр: длинная полуволна = P + δ, короткая = P - δ,
    // δ = 7P/8. Получаются полупериоды 15P/8 и P/8 (отношение 15:1, тембр
    // близок к исходному), сумма = 2P - то же, что у sound() для данного P,
    // т.е. нота играет на названной частоте без сдвига на четверть тона.
    // period передаётся полным (NOTE/N), без дополнительного делителя NV.
    asm volatile (
        "mov %[period], r1\n\t"      // r1 = P
        "mov %[durance], r2\n\t"
        "mov $0100, r3\n\t"
        "mov r1, r5\n\t"
        "asr r5\n\t"
        "asr r5\n\t"
        "asr r5\n\t"                 // r5 = P/8
        "neg r5\n\t"                 // r5 = -P/8
        "add r1, r5\n\t"             // r5 = P - P/8 = 7P/8 = δ
        "clr r0\n"
".l1_%=:\n\t"
        "mov r1, r4\n\t"             // r4 = P
        "mov r0, @$-062\n\t"
        "bit $0100, r0\n\t"
        "beq .l_long_%=\n\t"
        "sub r5, r4\n\t"             // бит 6 был 1: SHORT = P - δ = P/8
        "br .l_inner_%=\n"
".l_long_%=:\n\t"
        "add r5, r4\n"               // бит 6 был 0: LONG = P + δ = 15P/8
".l_inner_%=:\n\t"
        "sob r4, .l_inner_%=\n\t"
        "xor r3, r0\n\t"
        "sob r2, .l1_%=\n\t"
        :
        : [period]"g"(period), [durance]"g"(durance)
        : "r0", "r1", "r2", "r3", "r4", "r5", "cc", "memory"
    );
}
