#include "memory.h"
#include "sprites.h"
#include "sound.h"
#include "tools.h"
#include "emt.h"
#include "digger_sprites.h"
#include "digger_full_font.h"
#include "digger_music.h"

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
constexpr uint16_t windmill_height = 32;
constexpr uint16_t table_height = SCREEN_PIX_HEIGHT - (str_height + y_space) - windmill_height;

const char digger_str[] = "D I G G E R";
constexpr uint16_t digger_str_x_pos = (SCREEN_BYTE_WIDTH - char_width * sizeof(digger_str) + char_width) / 2;

const char one_str[] = "ONE";
constexpr uint16_t one_x_pos = (SCREEN_BYTE_WIDTH + SCREEN_BYTE_WIDTH / 2 - char_width * sizeof(one_str) + char_width) / 2;

const char player_str[] = "PLAYER";
constexpr uint16_t player_x_pos = (SCREEN_BYTE_WIDTH + SCREEN_BYTE_WIDTH / 2 - char_width * sizeof(player_str) + char_width) / 2;

uint16_t one_player_y;

/**
 * @brief Инициализация игры
 */
void init_game()
{
    sp_paint_brick_long(0, 0, SCREEN_BYTE_WIDTH, SCREEN_PIX_HEIGHT, 0); // Очистка экрана

    uint16_t y_pos = 0;
    print_str(digger_str, digger_str_x_pos, y_pos);
    y_pos += str_height + y_space;

    sp_paint_brick_long(0, y_pos, SCREEN_BYTE_WIDTH, 2, 0b11111111);
    y_pos += 2;

    one_player_y = y_pos + y_space;

    print_str(one_str, one_x_pos, one_player_y);
    one_player_y += str_height + y_space;

    print_str(player_str, player_x_pos, one_player_y);

    one_player_y += str_height + y_space * 2;

    sp_paint_brick_long(0, y_pos, 1, table_height, 0b00000011);
    sp_paint_brick_long(SCREEN_BYTE_WIDTH - 1, y_pos, 1, table_height, 0b11000000);
    sp_paint_brick_long(SCREEN_BYTE_WIDTH / 2, y_pos, 1, table_height, 0b11000000);
    sp_paint_brick_long(SCREEN_BYTE_WIDTH / 2 + 1, y_pos, 1, table_height, 0b00000011);
    y_pos += table_height;

    sp_paint_brick_long(0, y_pos, SCREEN_BYTE_WIDTH, 2, 0b11111111);

}

uint16_t *cur_music_ptr = nullptr;
uint16_t demo_time = 0;
uint16_t nobbin_x = 0, nobbin_y = 0;
uint16_t hobbin_x = 0, hobbin_y = 0;
uint16_t digger_x = 0, digger_y = 0;
bool hobbin_mirror, digger_mirror;
uint8_t image_phase;        ///< Фаза анимации при выводе спрайта
int8_t image_phase_inc = 1; ///< Направление изменения фазы анимации при выводе спрайта (+1 или -1)

/**
 * @brief Обработка общего состояния игры (переход на новый уровень, Game Over и т.д.)
 */
void process_game_state()
{
    constexpr uint16_t image_width = sizeof(image_nobbin[0][0]);
    constexpr uint16_t image_height = sizeof(image_nobbin[0]) / sizeof(image_nobbin[0][0]); // Высота спрайта Ноббина
    constexpr uint16_t y_space = 8;

    constexpr uint16_t start_time = 0;
    constexpr uint16_t start_delay = 30;
    constexpr uint16_t move_durance = 20;
    constexpr uint16_t end_to_print = 6;

    // Тайминги отображения Ноббина в демо
    constexpr uint16_t nobbin_start_time = start_time + start_delay;
    constexpr uint16_t nobbin_begin_time = nobbin_start_time + 1;
    constexpr uint16_t nobbin_end_time = nobbin_begin_time + move_durance;
    constexpr uint16_t nobbin_print_time = nobbin_end_time + end_to_print;

    // Тайминги отображения Хоббина в демо
    constexpr uint16_t hobbin_start_time = nobbin_print_time + 6;
    constexpr uint16_t hobbin_begin_time = hobbin_start_time + 1;
    constexpr uint16_t hobbin_end_time = hobbin_begin_time + move_durance;
    constexpr uint16_t hobbin_mirror_time = hobbin_end_time + 1;
    constexpr uint16_t hobbin_print_time = hobbin_mirror_time + end_to_print;

    // Тайминги отображения Диггера в демо
    constexpr uint16_t digger_start_time = hobbin_print_time + 6;
    constexpr uint16_t digger_begin_time = digger_start_time + 1;
    constexpr uint16_t digger_end_time = digger_begin_time + move_durance;
    constexpr uint16_t digger_mirror_time = digger_end_time + 1;
    constexpr uint16_t digger_print_time = digger_mirror_time + end_to_print;

    switch (demo_time)
    {
        case start_time:
        {
            // Очистка области Demo
            sp_paint_brick_long(SCREEN_BYTE_WIDTH / 2 + 2, one_player_y, SCREEN_BYTE_WIDTH / 2 - 3, table_height - one_player_y, 0);
            break;
        }

        case nobbin_start_time:
        {
            nobbin_x = SCREEN_BYTE_WIDTH - 7;
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
            hobbin_x = SCREEN_BYTE_WIDTH - 7;
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
            digger_x = SCREEN_BYTE_WIDTH - 7;
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
    }

    if (nobbin_x)
    {
        sp_paint_brick_long(nobbin_x + image_width, nobbin_y, 1, image_height, 0);
        sp_4_15_put(nobbin_x, nobbin_y, (uint8_t *)image_nobbin[image_phase]);
    }

    if (hobbin_x)
    {
        sp_paint_brick_long(hobbin_x + image_width, hobbin_y, 1, image_height, 0);
        if (hobbin_mirror) sp_4_15_h_mirror_put(hobbin_x, hobbin_y, (uint8_t *)image_hobbin_right[image_phase]);
        else sp_4_15_put(hobbin_x, hobbin_y, (uint8_t *)image_hobbin_right[image_phase]);
    }

    if (digger_x)
    {
        sp_paint_brick_long(digger_x + image_width, digger_y, 1, image_height, 0);
        if (digger_mirror) sp_4_15_h_mirror_put(digger_x, digger_y, (uint8_t *)image_digger_right[image_phase]);
        else sp_4_15_put(digger_x, digger_y, (uint8_t *)image_digger_right[image_phase]);
    }

    // Увеличить/уменьшить фазу на единицу
    image_phase += image_phase_inc;

    // Переключить направление изменения фазы, если фаза дошла до предельного значения
    if (!image_phase || image_phase >= 2) image_phase_inc = -image_phase_inc;

    // Увеличивать время Demo по кругу
    if (++demo_time > 250) demo_time = 0;

    play_music((uint16_t *)music0, &cur_music_ptr);
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

    volatile uint16_t *t_limit = (volatile uint16_t *)REG_TVE_LIMIT;
    volatile union TVE_CSR *tve_csr = (volatile union TVE_CSR *)REG_TVE_CSR;

    constexpr uint16_t FPS = 50; // Частота обновления кадров
    *t_limit = 3000000 / 128 / 4 / FPS;

    init_game(); // Начальная инициализация игры

    for (;;) // Основной бесконечный цикл игры
    {
        // Настроить таймер на использование мониторинга события, включить счётчик и включить делитель на 4,
        // а так же, сбросить флаг события таймера
        tve_csr->reg = (1 << TVE_CSR_MON) | (1 << TVE_CSR_RUN) | (1 << TVE_CSR_D4);

        process_game_state();

        while ((tve_csr->reg & (1 << TVE_CSR_FL)) == 0); // Ожидать срабатывания таймера.
    }
}
