#include "sprites.h"
#include "memory.h"

// Объявляем сервисную процедуру звукового движка из title.c, чтобы можно
// было вызывать её из inline-asm ниже. Сама проверка FL встроена прямо во
// внешний цикл блиттера — в духе R5-полла из VALLEY.BIN.
extern void music_service();

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

        // Музыкальный тик после каждой строки — поддерживает звук во время
        // длинных заливок (например, очистки экрана 64×256). Сохраняем
        // только r0/r1 (caller-save в PDP-11 GCC ABI); r2 (счётчик строк)
        // и r4 (видеоуказатель) сохранятся прологом music_service.
        "tstb @%[csr]\n\t"
        "bpl .l_no_tick_%=\n\t"
        "mov r0, -(sp)\n\t"
        "mov r1, -(sp)\n\t"
        "jsr pc, _music_service\n\t"
        "mov (sp)+, r1\n\t"
        "mov (sp)+, r0\n"
".l_no_tick_%=:\n\t"

        "sob r2, .l1_%=\n\t"
        :
        : [x]"g"(x), [y]"g"(y), [x_width]"g"(x_width), [y_width]"g"(y_width), [color]"g"(color), [csr]"i"(REG_TVE_CSR)
        : "r0", "r1", "r2", "r3", "r4", "cc", "memory"
    );
}
