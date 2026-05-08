#include "sprites.h"
#include "memory.h"

// Объявляем сервисную процедуру звукового движка из title.c — её вызываем
// из встроенного быстрого пути (tstb @CSR / bpl / jsr) в строковом цикле
// блиттера. Так FL ловится не реже раза на строку, и музыка не теряет события.
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

        // Музыкальный тик после каждой строки. Быстрый путь — две инструкции
        // (tstb / bpl). Сохраняем r0/r1 вокруг возможного вызова service —
        // r2 (счётчик строк) и r4 (видеоуказатель) callee-save в PDP-11 ABI.
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
        : [x]"g"(x), [y]"g"(y), [x_width]"g"(x_width), [y_width]"g"(y_width), [color]"g"(color),
          [csr]"i"(REG_TVE_CSR)
        : "r0", "r1", "r2", "r3", "r4", "cc", "memory"
    );
}

// Ниже — title-версии стандартных блиттеров из sprites.c со встроенным
// `tstb @CSR / bpl / jsr _music_service` в начале каждой итерации внешнего
// цикла. Это гарантирует, что FL-событие таймера не теряется во время
// длинного блита спрайта, и тайминг мелодии не зависит от количества спрайтов.
//
// Линкер выбирает эти версии вместо sprites.c-версий благодаря тому, что
// sprites_title.o передаётся ld явно перед libs.a в title-сборке.

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
        // Music tick: r0 будет загружен заново ниже, сохранять не нужно.
        "tstb @#0177712\n\t"
        "bpl .l_mt1_%=\n\t"
        "jsr pc, _music_service\n"
".l_mt1_%=:\n\t"
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
        "tstb @#0177712\n\t"
        "bpl .l_mt2_%=\n\t"
        "jsr pc, _music_service\n"
".l_mt2_%=:\n\t"
        "mov (r3)+, (r4)+\n\t"
        "mov (r3)+, (r4)\n\t"
        "add $62, r4\n\t"
        "sob r2, .l2_%=\n"

".l3_%=:\n\t"
        :
        : [x]"g"(x), [y]"g"(y), [image]"m"(image)
        : "r0", "r1", "r2", "r3", "r4", "cc", "memory"
    );
}

// sp_4_15_h_mirror_put в title-сборке не нужен: Диггер рисуется через
// предзеркалированные данные image_digger_left из ROM, поэтому
// горячий цикл всегда использует быстрый sp_4_15_put. Версия из
// sprites.c (для диггерной сборки) остаётся нетронутой.

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
        // Music tick: r0 ниже перезаписывается, r1/r2/r5 callee-save сохранит сама.
        "tstb @#0177712\n\t"
        "bpl .L_mt_b_%=\n\t"
        "jsr pc, _music_service\n"
".L_mt_b_%=:\n\t"
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
        "tstb @#0177712\n\t"
        "bpl .L_mt_i_%=\n\t"
        "jsr pc, _music_service\n"
".L_mt_i_%=:\n\t"
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
        "tstb @#0177712\n\t"
        "bpl .L_mt_g_%=\n\t"
        "jsr pc, _music_service\n"
".L_mt_g_%=:\n\t"
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
