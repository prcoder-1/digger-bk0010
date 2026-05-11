#include "sprites.h"
#include "memory.h"

extern void music_service();

// Инлайновая проверка FL: тестируем старший бит CSR таймера и, если взведён,
// вызываем music_service напрямую — без обёртки music_tick (минус 15 циклов
// jsr+rts на каждый поллинг). Используется внутри спрайтовых asm-блоков,
// где мы и так вынуждены сохранять/перечитывать нужные регистры.
#define POLL_FL                              \
    "tstb @%[csr]\n\t"                        \
    "bpl 1f\n\t"                              \
    "jsr pc, _music_service\n"                \
    "1:\n\t"

// Версия POLL_FL для контекстов, где живы r0 и r1 (caller-saved). На «холодном»
// пути (FL не взведён) — всего tstb+bpl, без накладных. На «горячем» пути
// (FL взведён) — push/pop r0+r1 вокруг jsr pc, _music_service.
// 7 циклов на каждой проверке, 30+service_time на каждом service.
#define POLL_FL_SAVE_R0R1                    \
    "tstb @%[csr]\n\t"                        \
    "bpl 1f\n\t"                              \
    "mov r0, -(sp)\n\t"                       \
    "mov r1, -(sp)\n\t"                       \
    "jsr pc, _music_service\n\t"              \
    "mov (sp)+, r1\n\t"                       \
    "mov (sp)+, r0\n"                         \
    "1:\n\t"

void title_sp_clear_strip(uint16_t x, uint16_t y, uint16_t height)
{
    // Очистка полосы шириной 1 байт. Используются только r2 (callee-saved счётчик)
    // и r4 (callee-saved указатель), поэтому music_service можно вызывать без
    // сохранения каких-либо регистров.
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
        "mov %[height], r2\n\t"
".loop_%=:\n\t"
        "clrb (r4)+\n\t"
        "add $63, r4\n\t"
        // Подкладка nop-ами: цикл шире (~30 циклов) ближе к остальным
        // полленг-точкам, чтобы дисперсия задержки FL→service была одинаковой
        // во всех фазах работы (с/без спрайтов на экране).
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        POLL_FL
        "sob r2, .loop_%=\n\t"
        :
        : [x]"g"(x), [y]"g"(y), [height]"g"(height),
          [csr]"i"(REG_TVE_CSR)
        : "r0", "r1", "r2", "r4", "cc", "memory"
    );
}

void title_sp_paint_brick_long(uint16_t x, uint16_t y, uint16_t x_width, uint16_t y_width, uint8_t color)
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
        // Inline-проверка FL внутри inner loop с сохранением r0/r1 на горячем
        // пути. На широких очистках (case start_time: x_width≈29) без этого
        // получался 150-цикловой «слепой» участок на каждую строку — слышно
        // как обрыв звука на высоких нотах. Теперь проверяем после каждого
        // байта; на холодном пути это всего 7 циклов накладных.
        POLL_FL_SAVE_R0R1
        "sob r3, .l2_%=\n\t"

        "add $64, r4\n\t"
        "sub r1, r4\n\t"
        POLL_FL_SAVE_R0R1
        "sob r2, .l1_%=\n\t"
        :
        : [x]"g"(x), [y]"g"(y), [x_width]"g"(x_width), [y_width]"g"(y_width), [color]"g"(color),
          [csr]"i"(REG_TVE_CSR)
        : "r0", "r1", "r2", "r3", "r4", "cc", "memory"
    );
}

void title_sp_4_15_put(uint16_t x, uint16_t y, const uint8_t *image)
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
        POLL_FL
        "mov (r3)+, r0\n\t"
        "movb r0, (r4)+\n\t"
        "swab r0\n\t"
        "movb r0, (r4)\n\t"
        "add $61, r4\n\t"
        POLL_FL
        "sob r2, .l1_%=\n\t"
        "br .l4_%=\n"

".l2_%=:\n\t"
        "mov (r3)+, (r4)+\n\t"
        // Подкладка nop-ами до ~25 циклов, чтобы интервал между этим POLL_FL
        // и следующим был сопоставим с другими полленг-точками. Это уменьшает
        // зависимость дрожания от того, в какой фазе кода находится FL-событие.
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        POLL_FL
        "mov (r3)+, (r4)\n\t"
        "add $62, r4\n\t"
        POLL_FL
        "sob r2, .l2_%=\n"
".l4_%=:\n\t"
        :
        : [x]"g"(x), [y]"g"(y), [image]"m"(image),
          [csr]"i"(REG_TVE_CSR)
        : "r0", "r1", "r2", "r3", "r4", "cc", "memory"
    );
}

void title_sp_put(uint16_t x, uint16_t y, uint16_t x_width, uint16_t y_width, const uint8_t *image, const uint8_t *outline)
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
        "tstb @%[csr]\n\t" \
        "bpl 1f\n\t" \
        "jsr pc, _music_service\n" \
"1:\n\t" \
        "sob r3, .L_b_x_%=\n\t"
        "add $64, r5\n\t"
        "sob r4, .L_b_y_%=\n\t"
        "br .L_done_%=\n"

".L_img_only_%=:\n\t"
        // Перенос image ptr в r2 (r2 здесь 0 — мы попали в .L_img_only_
        // именно потому что outline=NULL). r2 callee-saved через music_service,
        // в отличие от r1, поэтому inline POLL_FL не сломает указатель.
        "mov r1, r2\n"
".L_i_y_%=:\n\t"
        "mov %[x_width], r3\n\t"
        "sub r3, r5\n"
".L_i_x_%=:\n\t"
        "movb (r2)+, (r5)+\n\t"
        "sob r3, .L_i_x_%=\n\t"
        "add $64, r5\n\t"
        POLL_FL
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
        : [x]"g"(x), [y]"g"(y), [x_width]"g"(x_width), [y_width]"g"(y_width), [image]"m"(image), [outline]"m"(outline),
          [csr]"i"(REG_TVE_CSR)
        : "r0", "r1", "r2", "r3", "r4", "r5", "cc", "memory"
    );
}
