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
        "mov (r3)+, (r4)+\n\t"
        "mov (r3)+, (r4)\n\t"
        "add $62, r4\n\t"
        "sob r2, .l2_%=\n"

".l3_%=:\n\t"
        :
        : [x]"g"(x), [y]"g"(y), [image]"m"(image)
        : "r0", "r2", "r3", "r4", "cc", "memory"
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

        "mov %[image], r1\n\t"
        "mov %[outline], r2\n\t"
        "mov %[y_width], r4\n\t"

        "tst r1\n\t"
        "beq .L_no_img_%=\n\t"
        "tst r2\n\t"
        "bne .L_both_%=\n\t"
        "br  .L_img_only_%=\n"
".L_no_img_%=:\n\t"
        "br  .L_generic_%=\n"

".L_both_%=:\n"
".L_b_y_%=:\n\t"
        "mov %[x_width], r3\n\t"
        "sub r3, r5\n"
".L_b_x_%=:\n\t"
        "movb (r5), r0\n\t"
        "bicb (r2)+, r0\n\t"
        "bisb (r1)+, r0\n\t"
        "movb r0, (r5)+\n\t"
        "sob r3, .L_b_x_%=\n\t"
        "add $64, r5\n\t"
        "sob r4, .L_b_y_%=\n\t"
        "br .L_done_%=\n"

".L_img_only_%=:\n"
".L_i_y_%=:\n\t"
        "mov %[x_width], r3\n\t"
        "sub r3, r5\n"
".L_i_x_%=:\n\t"
        "movb (r1)+, (r5)+\n\t"
        "sob r3, .L_i_x_%=\n\t"
        "add $64, r5\n\t"
        "sob r4, .L_i_y_%=\n\t"
        "br .L_done_%=\n"

".L_generic_%=:\n"
".L_g_y_%=:\n\t"
        "mov %[x_width], r3\n\t"
        "sub r3, r5\n"
".L_g_x_%=:\n\t"
        "clr r0\n\t"
        "tst r2\n\t"
        "beq .L_g_no_out_%=\n\t"
        "movb (r5), r0\n\t"
        "bicb (r2)+, r0\n"
".L_g_no_out_%=:\n\t"
        "tst r1\n\t"
        "beq .L_g_no_img_%=\n\t"
        "bisb (r1)+, r0\n"
".L_g_no_img_%=:\n\t"
        "movb r0, (r5)+\n\t"
        "sob r3, .L_g_x_%=\n\t"
        "add $64, r5\n\t"
        "sob r4, .L_g_y_%=\n"

".L_done_%=:\n\t"
        :
        : [x]"g"(x), [y]"g"(y), [x_width]"g"(x_width), [y_width]"g"(y_width), [image]"m"(image), [outline]"m"(outline)
        : "r0", "r1", "r2", "r3", "r4", "r5", "cc", "memory"
    );
}

