#include "tools.h"
#include "memory.h"

uint16_t abs(const int16_t x)
{
    uint16_t rv;
    asm(
        "cmp %[x], $0\n\t"
        "bge .l_quit%=\n\t"
        "neg %[x]\n"
".l_quit%=:\n\t"
        "rts pc\n\t"
        : "=r" (rv)
        : [x]"r"(x)
    );

    return rv;
}

uint8_t absb(const int8_t x)
{
    uint8_t rv;
    asm(
        "cmpb %[x], $0\n\t"
        "bge .l_quit%=\n\t"
        "negb %[x]\n"
".l_quit%=:\n\t"
        "rts pc\n\t"
        : "=r" (rv)
        : [x]"r"(x)
    );

    return rv;
}

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

void uint_to_str(uint16_t value, char **str)
{
    asm(
        "mov %[str], r3\n\t"
        "mov (r3), r1\n\t"
        "mov %[value], r0\n\t"
".l1_%=:\n\t"
        "clr r2\n"
".l2_%=:\n\t"
        "inc r2\n\t"
        "sub $10, r0\n\t"
        "bhis .l2_%=\n\t"
        "add $58, r0\n\t"
        "movb r0, -(r1)\n\t"
        "mov r2, r0\n\t"
        "sob r0, .l1_%=\n\t"
        "mov r1, (r3)\n\t"
        :
        : [value]"r" (value), [str]"m"(str)
        : "r0", "r1", "r2", "r3", "cc", "memory"
    );
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
