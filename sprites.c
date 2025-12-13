#include "sprites.h"
#include "memory.h"

void sp_4_15_put(uint16_t x, uint16_t y, const uint8_t *image)
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

        "mov %[image], r3\n\t"
        "mov $15, r2\n\t"

        "bit r4, $1\n\t"
        "beq .l2_%=\n"

".l1_%=:\n\t"
        "mov (r3)+, r0\n\t"
        "movb r0, (r4)+\n\t"
        "swab r0\n\t"
        "movb r0, (r4)+\n\t"
        "mov (r3)+, r0\n\t"
        "movb r0, (r4)+\n\t"
        "swab r0\n\t"
        "movb r0, (r4)\n\t"
        "add $61, r4\n\t"
        "sob r2, .l1_%=\n\t"
        "br .l3_%=\n"

".l2_%=:\n\t"
        "mov (r3)+, r0\n\t"
        "mov r0, (r4)+\n\t"
        "mov (r3)+, r0\n\t"
        "mov r0, (r4)\n\t"
        "add $62, r4\n\t"
        "sob r2, .l2_%=\n"

".l3_%=:\n\t"
        :
        : [x]"g"(x), [y]"g"(y), [image]"m"(image)
        : "r0", "r2", "r3", "r4", "cc", "memory"
    );
}

void sp_4_15_h_mirror_put(uint16_t x, uint16_t y, const uint8_t *image)
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
        "add $4, r4\n\t"

        "mov %[image], r3\n\t"
        "mov $15, r2\n\t"

        "bit r4, $1\n\t"
        "beq .l2_%=\n"

".l1_%=:\n\t"
        "mov (r3)+, r0\n\t"
        "jsr pc, .l_mirror_%=\n\t"
        "movb r0, -(r4)\n\t"
        "swab r0\n\t"
        "movb r0, -(r4)\n\t"

        "mov (r3)+, r0\n\t"
        "jsr pc, .l_mirror_%=\n\t"
        "movb r0, -(r4)\n\t"
        "swab r0\n\t"
        "movb r0, -(r4)\n\t"

        "add $68, r4\n\t"
        "sob r2, .l1_%=\n\t"
        "br .lq_%=\n"

".l2_%=:\n\t"
        "add r4, $6\n"

".l3_%=:\n\t"
        "mov (r3)+, r0\n\t"
        "jsr pc, .l_mirror_%=\n\t"
        "swab r0\n\t"
        "mov r0, -(r4)\n\t"

        "mov (r3)+, r0\n\t"
        "jsr pc, .l_mirror_%=\n\t"
        "swab r0\n\t"
        "mov r0, -(r4)\n\t"

        "add $68, r4\n\t"
        "sob r2, .l3_%=\n\t"
        "br .lq_%=\n"

".l_mirror_%=:\n\t"
        // 16-bit word mirror
        "mov r0, r1\n\t"      // r0 = r1 = ABCD EFGH
        "bic $0x3333, r1\n\t" //      r1 = 0B0D 0F0H
        "bic r1, r0\n\t"      //      r0 = A0C0 E0G0

        "asl r0\n\t"
        "asl r0\n\t"          //      r0 = 0A0C 0E0G

        "clc\n\t"
        "ror r1\n\t"
        "ror r1\n\t"          //      r1 = B0D0 F0H0

        "bis r1, r0\n\t"      //      r0 = BADC FEGH
        "mov r0, r1\n\t"      //      r1 = BADC FEGH

        "bic $0xF0F0, r1\n\t" //      r1 = 00DC 00GH
        "bic r1, r0\n\t"      //      r0 = BA00 FE00

        "asl r1\n\t"
        "asl r1\n\t"
        "asl r1\n\t"
        "asl r1\n\t"          //      r0 = 00BA 00FE

        "clc\n\t"
        "ror r0\n\t"
        "ror r0\n\t"
        "ror r0\n\t"
        "ror r0\n\t"          //      r1 = DC00 GH00

        "bis r1, r0\n\t"      //      r0 = DCBA GHFE
        "rts pc\n"

".lq_%=:\n\t"
        :
        : [x]"g"(x), [y]"g"(y), [image]"m"(image)
        : "r0", "r1", "r2", "r3", "r4", "r5", "cc", "memory"
    );
}

void sp_4_15_hv_mirror_put(uint16_t x, uint16_t y, const uint8_t *image)
{
    asm(
        "mov $040000, r4\n\t"
        "mov %[y], r0\n\t"
        "add $14, r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "add r0, r4\n\t"
        "add %[x], r4\n\t"
        "add $4, r4\n\t"

        "mov %[image], r3\n\t"
        "mov $15, r2\n\t"

        "bit r4, $1\n\t"
        "beq .l2_%=\n"

".l1_%=:\n\t"
        "mov (r3)+, r0\n\t"
        "jsr pc, .l_mirror_%=\n\t"
        "movb r0, -(r4)\n\t"
        "swab r0\n\t"
        "movb r0, -(r4)\n\t"

        "mov (r3)+, r0\n\t"
        "jsr pc, .l_mirror_%=\n\t"
        "movb r0, -(r4)\n\t"
        "swab r0\n\t"
        "movb r0, -(r4)\n\t"

        "sub $60, r4\n\t"
        "sob r2, .l1_%=\n\t"
        "br .lq_%=\n"

".l2_%=:\n\t"
        "add r4, $6\n"

".l3_%=:\n\t"
        "mov (r3)+, r0\n\t"
        "jsr pc, .l_mirror_%=\n\t"
        "swab r0\n\t"
        "mov r0, -(r4)\n\t"

        "mov (r3)+, r0\n\t"
        "jsr pc, .l_mirror_%=\n\t"
        "swab r0\n\t"
        "mov r0, -(r4)\n\t"

        "sub $60, r4\n\t"
        "sob r2, .l3_%=\n\t"
        "br .lq_%=\n"

".l_mirror_%=:\n\t"
        // 16-bit word mirror
        "mov r0, r1\n\t"      // r0 = r1 = ABCD EFGH
        "bic $0x3333, r1\n\t" //      r1 = 0B0D 0F0H
        "bic r1, r0\n\t"      //      r0 = A0C0 E0G0

        "asl r0\n\t"
        "asl r0\n\t"          //      r0 = 0A0C 0E0G

        "clc\n\t"
        "ror r1\n\t"
        "ror r1\n\t"          //      r1 = B0D0 F0H0

        "bis r1, r0\n\t"      //      r0 = BADC FEGH
        "mov r0, r1\n\t"      //      r1 = BADC FEGH

        "bic $0xF0F0, r1\n\t" //      r1 = 00DC 00GH
        "bic r1, r0\n\t"      //      r0 = BA00 FE00

        "asl r1\n\t"
        "asl r1\n\t"
        "asl r1\n\t"
        "asl r1\n\t"          //      r0 = 00BA 00FE

        "clc\n\t"
        "ror r0\n\t"
        "ror r0\n\t"
        "ror r0\n\t"
        "ror r0\n\t"          //      r1 = DC00 GH00

        "bis r1, r0\n\t"      //      r0 = DCBA GHFE
        "rts pc\n"

".lq_%=:\n\t"
        :
        : [x]"g"(x), [y]"g"(y), [image]"m"(image)
        : "r0", "r1", "r2", "r3", "r4", "cc", "memory"
    );
}

void sp_put(uint16_t x, uint16_t y, uint16_t x_width, uint16_t y_width, const uint8_t *image, const uint8_t *outline)
{
    asm(
        "mov $040000, r5\n\t"

        "mov %[y], r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "add r0, r5\n\t"
        "add %[x], r5\n\t"
        "add %[x_width], r5\n"

        "mov %[image], r1\n\t"   // r1 = image
        "mov %[outline], r2\n\t" // r2 = outline
        "mov %[y_width], r4\n"   // r4 = y_width

".l_y_loop_%=:\n\t"
        "mov %[x_width], r3\n\t" // r3 = x_width
        "sub r3, r5\n"

".l_x_loop_%=:\n\t"
        "clr r0\n\t"
        "tst r2\n\t"
        "beq .l1_%=\n\t"
        "movb (r5), r0\n"
        "bicb (r2)+, r0\n"
".l1_%=:\n\t"
        "tst r1\n\t"
        "beq .l2_%=\n\t"
        "bisb (r1)+, r0\n"
".l2_%=:\n\t"
        "movb r0, (r5)+\n"
        "sob r3, .l_x_loop_%=\n\t"

        "movb %[x_width], r0\n\t"
        "add $64, r5\n"
        "sob r4, .l_y_loop_%=\n\t"

        :
        : [x]"g"(x), [y]"g"(y), [x_width]"g"(x_width), [y_width]"g"(y_width), [image]"m"(image), [outline]"m"(outline)
        : "r0", "r1", "r2", "r3", "r4", "r5", "cc", "memory"
    );
}

void sp_paint_brick(uint16_t x, uint16_t y, uint8_t x_width, uint8_t y_width, uint8_t color)
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
        "movb %[x_width], r1\n\t"
        "movb %[y_width], r2\n"
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
