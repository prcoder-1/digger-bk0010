#include "memory.h"
#include "sprites.h"
#include "sound.h"
#include "tools.h"
#include "emt.h"
#include "digger_sprites_title.h"
#include "digger_full_font.h"
#include "digger_music.h"

#define COIN_Y_OFFSET 3 // Смещение спрайта монетки в ячейке по оси Y

void sound_tmr(uint16_t period, uint8_t durance)
{
    asm volatile (
        "movb %[durance], r0\n\t"
        "asl r0\n\t"
        "asl r0\n\t"
        "mov r0, @%[REG_TVE_LIMIT]\n\t"
        "mov %[TIMER_MODE], @%[REG_TVE_CSR]\n"
        "mov %[period], r1\n\t"
        "mov $0100, r2\n\t"
        "clr r0\n\t"
".l1_%=:\n\t"
        "mov r1, r3\n\t"
        "mov r0, @$-062\n"
// ------- [Vibrato] -------
        "bit $0100, r0\n\t"
        "beq .l2_%=\n\t"
        "asr r4\n\t"
        "asr r4\n\t"
        "asr r4\n\t"
        "br .l3_%=\n\t"
".l2_%=:\n\t"
        "asl r4\n\t"
        "asl r4\n\t"
        "asl r4\n\t"
// ------- [Vibrato] -------
".l3_%=:\n\t"
        "sob r3, .l3_%=\n\t"
        "xor r2, r0\n\t"
        "movb @%[REG_TVE_CSR], r3\n\t"
        "tstb r3\n\t"
        "bge .l1_%=\n\t"
        :
        : [period]"g"(period), [durance]"g"(durance), [REG_TVE_LIMIT]"i"(REG_TVE_LIMIT), [REG_TVE_CSR]"i"(REG_TVE_CSR),
          [TIMER_MODE]"i"((1 << TVE_CSR_MON) | (1 << TVE_CSR_RUN) | (1 << TVE_CSR_D16) /*| (1 << TVE_CSR_D4)*/)
        : "r0", "r1", "r2", "r3", "cc", "memory"
    );
}

/**
 * @brief Вывод 16-битного десятичного числа
 *
 * @param number - число для вывода
 * @param x_graph - координата X по которой будет осуществлён вывод числа
 * @param y_graph - координата Y по которой будет осуществлён вывод числа
 */
void print_dec(uint16_t number, uint16_t x_graph, uint16_t y_graph)
{
    constexpr char zero = '0';
    char buf[5] = { zero, zero, zero, zero, zero }; // Буфер для 5 десятичных знаков

    char *ptr = &buf[sizeof(buf)];
    uint_to_str(number, &ptr);

    for (uint8_t i = 0; i < sizeof(buf); ++i)
    {
        uint16_t index = buf[i] - zero;
        sp_put(x_graph, y_graph, sizeof(ch_digits[0][0]), sizeof(ch_digits[0]) / sizeof(ch_digits[0][0]), (uint8_t *)ch_digits[index], nullptr); // Вывести спрайт цифры
        x_graph += sizeof(ch_digits[0][0]);
    }
}

/**
 * @brief Вывод строки
 *
 * @param str - строка для вывода
 * @param x_graph - координата X по которой будет осуществлён вывод числа
 * @param y_graph - координата Y по которой будет осуществлён вывод числа
 */
void print_str(const char *str, uint16_t x_graph, uint16_t y_graph)
{
    while (*str)
    {
        char c = *str++;
        if (c != ' ')
        {
            uint16_t index = c - 'A';
            sp_put(x_graph, y_graph, sizeof(ch_alpha[0][0]), sizeof(ch_alpha[0]) / sizeof(ch_alpha[0][0]), (uint8_t *)ch_alpha[index], nullptr); // Вывести спрайт буквы
        }

        x_graph += sizeof(ch_alpha[0][0]);
    }
}

constexpr uint16_t char_width = sizeof(ch_alpha[0][0]);
constexpr uint16_t str_height = sizeof(ch_alpha[0]) / char_width;
constexpr uint16_t y_space = 8;
constexpr uint16_t windmill_height = 42;
constexpr uint16_t table_height = SCREEN_PIX_HEIGHT - (str_height + y_space) - windmill_height;

const char digger_str[] = "D I G G E R";
constexpr uint16_t digger_str_x_pos = (SCREEN_BYTE_WIDTH - char_width * sizeof(digger_str) + char_width) / 2;

const char one_str[] = "ONE";
constexpr uint16_t one_x_pos = (SCREEN_BYTE_WIDTH + SCREEN_BYTE_WIDTH / 2 - char_width * sizeof(one_str) + char_width) / 2;

const char player_str[] = "PLAYER";
constexpr uint16_t player_x_pos = (SCREEN_BYTE_WIDTH + SCREEN_BYTE_WIDTH / 2 - char_width * sizeof(player_str) + char_width) / 2;

uint16_t one_player_y;
uint16_t note_index = 0;

/**
 * @brief Инициализация игры
 */
void init_demo()
{
    sp_paint_brick_long(0, 0, SCREEN_BYTE_WIDTH, SCREEN_PIX_HEIGHT, 0); // Очистка экрана

    uint16_t y_pos = 0;

    // Строка "D I G G E R"
    print_str(digger_str, digger_str_x_pos, y_pos);
    y_pos += str_height + y_space;

    // Верхняя линия рамки
    sp_paint_brick_long(0, y_pos, SCREEN_BYTE_WIDTH, 2, 0b01010101);
    y_pos += 2;

    one_player_y = y_pos + y_space;

    // Строка "ONE"
    print_str(one_str, one_x_pos, one_player_y);
    one_player_y += str_height + y_space;

    // Строка "PLAYER"
    print_str(player_str, player_x_pos, one_player_y);

    one_player_y += str_height + y_space * 2;

    // Вертикальные линии рамки и разделитель
    sp_paint_brick_long(0, y_pos, 1, table_height, 0b00000001);
    sp_paint_brick_long(SCREEN_BYTE_WIDTH - 1, y_pos, 1, table_height, 0b01000000);
    sp_paint_brick_long(SCREEN_BYTE_WIDTH / 2, y_pos, 1, table_height, 0b01000000);
    sp_paint_brick_long(SCREEN_BYTE_WIDTH / 2 + 1, y_pos, 1, table_height, 0b00000001);
    y_pos += table_height;

    // Нижняя линия рамки
    sp_paint_brick_long(0, y_pos, SCREEN_BYTE_WIDTH, 2, 0b01010101);
}

uint16_t *cur_music_ptr = nullptr;
uint16_t note_duration_count = 0;
uint16_t demo_time = 0;
uint16_t nobbin_x = 0, nobbin_y = 0;
uint16_t hobbin_x = 0, hobbin_y = 0;
uint16_t digger_x = 0, digger_y = 0;
uint16_t bag_x = 0, bag_y = 0;
uint16_t emerald_x = 0, emerald_y = 0;
uint16_t cherry_x = 0, cherry_y = 0;
bool hobbin_mirror, digger_mirror;
uint8_t image_phase;        ///< Фаза анимации при выводе спрайта
int8_t image_phase_inc = 1; ///< Направление изменения фазы анимации при выводе спрайта (+1 или -1)

/**
 * @brief Обработка общего состояния игры (переход на новый уровень, Game Over и т.д.)
 */
void process_demo_state()
{
    constexpr uint16_t image_width = sizeof(image_nobbin[0][0]);
    constexpr uint16_t image_height = sizeof(image_nobbin[0]) / sizeof(image_nobbin[0][0]); // Высота спрайта Ноббина
    constexpr uint16_t y_space = 8;
    constexpr uint16_t move_start_pos = SCREEN_BYTE_WIDTH - 6;

    constexpr uint16_t start_time = 0;
    constexpr uint16_t start_delay = 20;
    constexpr uint16_t move_durance = 22;
    constexpr uint16_t end_to_print = 4;
    constexpr uint16_t print_to_next = 6;

    // Тайминги отображения Ноббина в демо
    constexpr uint16_t nobbin_start_time = start_time + start_delay;
    constexpr uint16_t nobbin_begin_time = nobbin_start_time + 1;
    constexpr uint16_t nobbin_end_time = nobbin_begin_time + move_durance;
    constexpr uint16_t nobbin_print_time = nobbin_end_time + end_to_print;

    // Тайминги отображения Хоббина в демо
    constexpr uint16_t hobbin_start_time = nobbin_print_time + print_to_next;
    constexpr uint16_t hobbin_begin_time = hobbin_start_time + 1;
    constexpr uint16_t hobbin_end_time = hobbin_begin_time + move_durance;
    constexpr uint16_t hobbin_mirror_time = hobbin_end_time + 1;
    constexpr uint16_t hobbin_print_time = hobbin_mirror_time + end_to_print;

    // Тайминги отображения Диггера в демо
    constexpr uint16_t digger_start_time = hobbin_print_time + print_to_next;
    constexpr uint16_t digger_begin_time = digger_start_time + 1;
    constexpr uint16_t digger_end_time = digger_begin_time + move_durance;
    constexpr uint16_t digger_mirror_time = digger_end_time + 1;
    constexpr uint16_t digger_print_time = digger_mirror_time + end_to_print;

    // Тайминги отображения мешка в демо
    constexpr uint16_t bag_display_time = digger_print_time + print_to_next;
    constexpr uint16_t bag_print_time = bag_display_time + end_to_print;

    // Тайминги отображения монеты в демо
    constexpr uint16_t emerald_display_time = bag_print_time + print_to_next;
    constexpr uint16_t emerald_print_time = emerald_display_time + end_to_print;

    // Тайминги отображения вишенки в демо
    constexpr uint16_t cherry_display_time = emerald_print_time + print_to_next;
    constexpr uint16_t cherry_print_time = cherry_display_time + end_to_print;

    // Время до повтора демо
    constexpr uint16_t demo_restart_time = cherry_print_time + 50;

    volatile uint16_t *t_limit = (volatile uint16_t *)REG_TVE_LIMIT;
    volatile union TVE_CSR *tve_csr = (volatile union TVE_CSR *)REG_TVE_CSR;

    uint16_t duration = popcorn_durations[note_index];

    *t_limit = (17 - duration) << 1;
    tve_csr->reg = (1 << TVE_CSR_MON) | (1 << TVE_CSR_RUN) | (1 << TVE_CSR_D4);

    switch (demo_time)
    {
        case start_time:
        {
            // Очистка области Demo
            nobbin_x = hobbin_x = digger_x = 0;
            constexpr uint16_t demo_height = table_height - (str_height + y_space * 2) * 2;
            sp_paint_brick_long(SCREEN_BYTE_WIDTH / 2 + 2, one_player_y, SCREEN_BYTE_WIDTH / 2 - 3, demo_height, 0);
            break;
        }

        case nobbin_start_time:
        {
            nobbin_x = move_start_pos;
            nobbin_y = one_player_y;
            break;
        }

        case nobbin_begin_time ... nobbin_end_time:
        {
            nobbin_x--;
            break;
        }

        case nobbin_print_time:
        {
            const char nobbin_str[] = "NOBBIN";
            print_str(nobbin_str, nobbin_x + image_width * 2 - 1, nobbin_y);
            break;
        }

        case hobbin_start_time:
        {
            hobbin_mirror = true;
            hobbin_x = move_start_pos;
            hobbin_y = nobbin_y + image_height + y_space;
            break;
        }

        case hobbin_begin_time ... hobbin_end_time:
        {
            hobbin_x--;
            break;
        }

        case hobbin_mirror_time:
        {
            hobbin_mirror = false;
            break;
        }

        case hobbin_print_time:
        {
            const char hobbin_str[] = "HOBBIN";
            print_str(hobbin_str, hobbin_x + image_width * 2 - 1, hobbin_y);
            break;
        }

        case digger_start_time:
        {
            digger_mirror = true;
            digger_x = move_start_pos;
            digger_y = hobbin_y + image_height + y_space;
            break;
        }

        case digger_begin_time ... digger_end_time:
        {
            digger_x--;
            break;
        }

        case digger_mirror_time:
        {
            digger_mirror = false;
            break;
        }

        case digger_print_time:
        {
            const char digger_str[] = "DIGGER";
            print_str(digger_str, digger_x + image_width * 2 - 1, digger_y);
            break;
        }

        case bag_display_time:
        {
            bag_x = digger_x;
            bag_y = digger_y + image_height + y_space;
            sp_4_15_put(bag_x, bag_y, (uint8_t *)image_bag);
            break;
        }

        case bag_print_time:
        {
            const char digger_str[] = "GOLD";
            print_str(digger_str, bag_x + image_width * 2 - 1, bag_y);
            break;
        }

        case emerald_display_time:
        {
            emerald_x = bag_x;
            emerald_y = bag_y + image_height + y_space;
            sp_put(emerald_x, emerald_y + COIN_Y_OFFSET, sizeof(image_coin[0]), sizeof(image_coin) / sizeof(image_coin[0]), (uint8_t *)image_coin, nullptr);
            break;
        }

        case emerald_print_time:
        {
            const char emerald_str[] = "EMERALD";
            print_str(emerald_str, emerald_x + image_width * 2 - 1, emerald_y);
            break;
        }

        case cherry_display_time:
        {
            cherry_x = emerald_x;
            cherry_y = emerald_y + image_height + y_space;
            sp_4_15_put(cherry_x, cherry_y, (uint8_t *)image_cherry);
            break;
        }

        case cherry_print_time:
        {
            const char bonus_str[] = "BONUS";
            print_str(bonus_str, cherry_x + image_width * 2 - 1, cherry_y);
            break;
        }
    }

    if (nobbin_x)
    {
        sp_paint_brick_long(nobbin_x + image_width, nobbin_y, 1, image_height, 0);
        sp_4_15_put(nobbin_x, nobbin_y, (uint8_t *)image_nobbin[image_phase]);
    }

    if (hobbin_x)
    {
        sp_paint_brick_long(hobbin_x + image_width, hobbin_y, 1, image_height, 0);
        if (hobbin_mirror) sp_4_15_put(hobbin_x, hobbin_y, (uint8_t *)image_hobbin_left[image_phase]);
        else sp_4_15_put(hobbin_x, hobbin_y, (uint8_t *)image_hobbin_right[image_phase]);
    }

    if (digger_x)
    {
        sp_paint_brick_long(digger_x + image_width, digger_y, 1, image_height, 0);
        if (digger_mirror) sp_4_15_h_mirror_put(digger_x, digger_y, (uint8_t *)image_digger_right[image_phase]);
        else sp_4_15_put(digger_x, digger_y, (uint8_t *)image_digger_right[image_phase]);
    }

    if (demo_time & 3)
    {
        // Увеличить/уменьшить фазу на единицу
        image_phase += image_phase_inc;

        // Переключить направление изменения фазы, если фаза дошла до предельного значения
        if (!image_phase || image_phase >= 2) image_phase_inc = -image_phase_inc;
    }

    // Увеличивать время Demo по кругу
    if (++demo_time > demo_restart_time)
    {
        demo_time = 0;
    }

    while ((tve_csr->reg & (1 << TVE_CSR_FL)) == 0); // Ожидать срабатывания таймера.

    uint16_t period = popcorn_periods[note_index];
    if (note_duration_count++ >= 20)
    {
        note_duration_count = 0;
        note_index++;
    }

    if (!period || !duration) note_index = 0;
    else sound_tmr(period, duration);
}

extern void start();

/**
 * @brief Основная программа
 */
void main()
{
    EMT_14();

    typedef void (*vector)();
    *((volatile vector *)VEC_STOP) = start; // Установить вектор клавиши "СТОП" на _start

    EMT_16(0233);
    EMT_16(0236);

    set_PSW(1 << PSW_I); // Замаскировать прерывания IRQ
    ((union KEY_STATE *)REG_KEY_STATE)->bits.INT_MASK = 1; // Отключить прерывание от клавиатуры

    init_demo(); // Начальная инициализация игры
    for (;;) process_demo_state(); // Основной бесконечный цикл демо
}
