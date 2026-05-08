#include "sprites.h"
#include "memory.h"

void sp_paint_brick_long(uint16_t x, uint16_t y, uint16_t x_width, uint16_t y_width, uint8_t color)
{
    asm(
        "mov $040000, r4\n\t"
        "mov %[y], r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "add r0, r4\n\t"
        "add %[x], r4\n\t"
        "mov %[x_width], r1\n\t"
        "mov %[y_width], r2\n"
        "movb %[color], r0\n\t"
".l1_%=:\n\t"
        "mov r1, r3\n"
".l2_%=:\n\t"
        "movb r0, (r4)+\n\t"
        "sob r3, .l2_%=\n\t"

        "add $64, r4\n\t"
        "sub r1, r4\n\t"

        "sob r2, .l1_%=\n\t"
        :
        : [x]"g"(x), [y]"g"(y), [x_width]"g"(x_width), [y_width]"g"(y_width), [color]"g"(color)
        : "r0", "r1", "r2", "r3", "r4", "cc", "memory"
    );
}
