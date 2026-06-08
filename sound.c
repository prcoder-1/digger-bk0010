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

// Аудио через PWM при опорной частоте 1/(2*period). За один полный аудио-цикл
// (2*period sob-тактов) динамик включён на `pw` тактов, выключен на остальные
// (2*period - pw). Высота тона определяется period, а перцептивная громкость
// - дробью pw/(2*period). Используется для построения огибающей: вызывающий
// C-код делит ноту на несколько стадий и для каждой передаёт свой pw.
//
// Допустимый диапазон: 1 <= pw <= 2*period - 1. При нарушении внутренние sob
// получают 0 и крутят 65536 итераций - C-обёртка обязана клампить.
//
// durance считается так же, как в sound_vibrato (число полупериодов): здесь
// делится на 2, чтобы получить число полных аудио-циклов.
void sound_pwm(uint16_t period, uint16_t durance, uint16_t pw)
{
    asm volatile (
        "mov %[period], r1\n\t"
        "mov %[durance], r2\n\t"
        "mov %[pw], r5\n\t"
        "asr r2\n\t"                  // полупериоды -> полные циклы
        "beq .l_end_%=\n"             // меньше одного цикла - выход
".l1_%=:\n\t"
        "mov $0100, @$-062\n\t"       // динамик ON
        "mov r5, r4\n"
".l_on_%=:\n\t"
        "sob r4, .l_on_%=\n\t"
        "clr @$-062\n\t"              // динамик OFF
        "mov r1, r4\n\t"
        "asl r4\n\t"                  // r4 = 2*period
        "sub r5, r4\n"                // r4 = 2*period - pw
".l_off_%=:\n\t"
        "sob r4, .l_off_%=\n\t"
        "sob r2, .l1_%=\n"
".l_end_%=:\n\t"
        :
        : [period]"g"(period), [durance]"g"(durance), [pw]"g"(pw)
        : "r1", "r2", "r4", "r5", "cc", "memory"
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
