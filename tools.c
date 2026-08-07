#include "tools.h"

void delay_ms(uint16_t time) // TODO: калибровка задержки
{
    asm volatile (
        "mov %[time], r1\n"
".l1_%=:\n\t"
        "mov $128, r0\n\r"
".l2_%=:\n\t"
        "sob r0, .l2_%=\n\r"
        "sob r1, .l1_%=\n\t"
        : : [time]"r" (time) : "r0", "r1", "cc"
    );
}

uint16_t rand()
{
    static uint16_t lfsr = 0x1234U;
    lfsr ^= lfsr >> 7;
    lfsr ^= lfsr << 9;
    lfsr ^= lfsr >> 13;

    return lfsr;
}
