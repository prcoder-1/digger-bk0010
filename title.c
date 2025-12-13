#include "memory.h"
#include "sprites.h"
#include "sound.h"
#include "tools.h"
#include "emt.h"
#include "digger_sprites.h"
#include "digger_full_font.h"
#include "digger_music.h"

uint16_t *cur_music_ptr = nullptr;

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
void print_str(char *str, uint16_t x_graph, uint16_t y_graph)
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

/**
 * @brief Инициализация игры
 */
void init_game()
{
    sp_paint_brick(0, 0, SCREEN_BYTE_WIDTH, 12, 0b10101010); // Очистить экран с верха до начала игрового поля

    const uint16_t x_pos = SCREEN_BYTE_WIDTH / 2 - sizeof(ch_alpha[0][0]) * 11 / 2;
    print_str("D I G G E R", x_pos, 0);
}

/**
 * @brief Обработка общего состояния игры (переход на новый уровень, Game Over и т.д.)
 */
void process_game_state()
{
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

    constexpr uint16_t FPS = 10; // Частота обновления кадров
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
