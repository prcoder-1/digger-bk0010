#include "sound.h"
#include "memory.h"

uint16_t fx_mode; // SOUND_BEEPER

void sound(uint16_t period, uint16_t durance)
{
    uint16_t mask = 0100; // Бит пищалки; 0 - только выдержать время, звучит AY
    if (fx_mode == SOUND_AY)
    {
        ay_fx(period);
        mask = 0;
    }
    period = bk_loops(period, 8); // durance в полупериодах - длительность следует за периодом
    asm volatile (
        "mov %[period], r1\n\t"
        "bne .Lp_%=\n\t"          // period 0 -> 1: иначе sob r4 крутит 65536 итераций
        "inc r1\n"                // (зависание со щелчками при period == 0)
        ".Lp_%=:\n\t"
        "mov %[durance], r2\n\t"
        "mov %[mask], r3\n\t"
        "clr r0\n\t"
".l1_%=:\n\t"
        "mov r1, r4\n"
        "mov r0, @$-062\n\t"
        "xor r3, r0\n\t"
".l2_%=:\n\t"
        "sob r4, .l2_%=\n\t"
        "sob r2, .l1_%=\n\t"
        :
        : [period]"g"(period), [durance]"g"(durance), [mask]"g"(mask)
        : "r0", "r1", "r2", "r3", "r4", "cc", "memory"
    );
    if (!mask) ay_fx(0);
}
