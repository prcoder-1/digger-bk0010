#include "tools.h"
#include "memory.h"

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

/*
uint8_t int_to_str(uint16_t value, uint8_t base, uint8_t width, char pad_char, char *str)
{
    char buffer[16];

    uint8_t i = 0;

    do
    {
        uint16_t reminder = value % base;
        if (reminder > 9) reminder += 'A' - '9';
        buffer[i++] = '0' + reminder;
        value /= base;
    }
    while(value);

    while (i < width) buffer[i++] = pad_char;

    uint8_t rv = i;
    while(i) *str++ = buffer[--i];

    return rv;
}
*/
