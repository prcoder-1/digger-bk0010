#include "sound.h"
#include "memory.h"

uint16_t fx_mode; // SOUND_BEEPER

static uint16_t fx_dummy; // Сюда пишет цикл вместо пищалки, пока звучит AY

void sound(uint16_t period, uint16_t durance)
{
    // На AY тот же цикл выдерживает время, но пишет не в пищалку 0177716, а в ОЗУ:
    // AZБК на записи в 0177716 переключал звуковой выход на пищалку, и AY глох
    uint16_t port = 0177716;
    const uint16_t ay = fx_mode == SOUND_AY;
    if (ay)
    {
        ay_fx(period);
        port = (uint16_t)&fx_dummy;
    }
    period = bk_loops(period, 8); // durance в полупериодах - длительность следует за периодом
    asm volatile (
        "mov %[period], r1\n\t"
        "bne .Lp_%=\n\t"          // period 0 -> 1: иначе sob r4 крутит 65536 итераций
        "inc r1\n"                // (зависание со щелчками при period == 0)
        ".Lp_%=:\n\t"
        "mov %[durance], r2\n\t"
        "mov %[port], .Lw_%=+2\n\t" // адрес в команде записи цикла (такты те же)
        "mov $0100, r3\n\t"
        "clr r0\n\t"
".l1_%=:\n\t"
        "mov r1, r4\n"
".Lw_%=:\n\t"
        "mov r0, @$-062\n\t"
        "xor r3, r0\n\t"
".l2_%=:\n\t"
        "sob r4, .l2_%=\n\t"
        "sob r2, .l1_%=\n\t"
        :
        : [period]"g"(period), [durance]"g"(durance), [port]"g"(port)
        : "r0", "r1", "r2", "r3", "r4", "cc", "memory"
    );
    if (ay) ay_fx(0);
}
