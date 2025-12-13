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

uint16_t one_player_y;

/**
 * @brief Инициализация игры
 */
void init_game()
{
    const uint16_t char_width = sizeof(ch_alpha[0][0]);
    const uint16_t str_height = sizeof(ch_alpha[0]) / char_width;
    const uint16_t y_space = 8;
    const uint16_t windmill_height = 32;

    sp_paint_brick_long(0, 0, SCREEN_BYTE_WIDTH, SCREEN_PIX_HEIGHT, 0); // Очистка экрана

    char digger_str[] = "D I G G E R";
    const uint16_t digger_str_x_pos = (SCREEN_BYTE_WIDTH - char_width * sizeof(digger_str) + char_width) / 2;

    uint16_t y_pos = 0;
    print_str(digger_str, digger_str_x_pos, y_pos);
    y_pos += str_height + y_space;

    sp_paint_brick_long(0, y_pos, SCREEN_BYTE_WIDTH, 2, 0b11111111);
    y_pos += 2;

    one_player_y = y_pos + y_space;

    const char one_str[] = "ONE";
    const uint16_t one_x_pos = (SCREEN_BYTE_WIDTH + SCREEN_BYTE_WIDTH / 2 - char_width * sizeof(one_str) + char_width) / 2;
    print_str(one_str, one_x_pos, one_player_y);
    one_player_y += str_height + y_space;

    const char player_str[] = "PLAYER";
    const uint16_t player_x_pos = (SCREEN_BYTE_WIDTH + SCREEN_BYTE_WIDTH / 2 - char_width * sizeof(player_str) + char_width) / 2;
    print_str(player_str, player_x_pos, one_player_y);

    one_player_y += str_height + y_space * 2;

    const uint16_t table_height = SCREEN_PIX_HEIGHT - (str_height + y_space) - windmill_height;
    sp_paint_brick_long(0, y_pos, 1, table_height, 0b00000011);
    sp_paint_brick_long(SCREEN_BYTE_WIDTH - 1, y_pos, 1, table_height, 0b11000000);
    sp_paint_brick_long(SCREEN_BYTE_WIDTH / 2, y_pos, 1, table_height, 0b11000000);
    sp_paint_brick_long(SCREEN_BYTE_WIDTH / 2 + 1, y_pos, 1, table_height, 0b00000011);
    y_pos += table_height;

    sp_paint_brick_long(0, y_pos, SCREEN_BYTE_WIDTH, 2, 0b11111111);

}

uint16_t *cur_music_ptr = nullptr;
uint16_t demo_time = 0;
uint16_t demo_x, demo_y;
uint8_t image_phase;       ///< Фаза анимации при выводе спрайта
int8_t image_phase_inc;    ///< Направление изменения фазы анимации при выводе спрайта (+1 или -1)

/**
 * @brief Обработка общего состояния игры (переход на новый уровень, Game Over и т.д.)
 */
void process_game_state()
{
    constexpr uint16_t image_width = sizeof(image_nobbin[0][0]);
    constexpr uint16_t image_height = sizeof(image_nobbin[0]) / sizeof(image_nobbin[0][0]); // Высота спрайта Ноббина

    switch (demo_time)
    {
        case 0:
        {
            // Очистка области Demo
            break;
        }

        case 50:
        {
            demo_x = SCREEN_BYTE_WIDTH - 7;
            demo_y = one_player_y;
        }

        case 51 ... 71:
        {
            sp_paint_brick_long(demo_x + image_width, demo_y, 1, image_height, 0);
            sp_4_15_put(demo_x, demo_y, (uint8_t *)image_nobbin[image_phase]);
            demo_x--;
            break;
        }

        case 77:
        {
            const char nobbin_str[] = "NOBBIN";
            print_str(nobbin_str, demo_x + image_width * 2 - 1, demo_y);
            break;
        }
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

    constexpr uint16_t FPS = 15; // Частота обновления кадров
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
