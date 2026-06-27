#include "memory.h"
#include "sprites.h"
#include "sound.h"
#include "tools.h"
#include "emt.h"
#include "digger_sprites_title.h"
#include "digger_music_title.h"
#include "digger_full_font.h"
#include "digger_title.h"

#define COIN_Y_OFFSET 3 // Смещение спрайта монетки в ячейке по оси Y
#define STR(x) #x

// Длительность одного «кадра» демо в тактах таймера (23438 Гц).
// 300 тактов ≈ 12.8 мс/кадр
constexpr uint16_t FRAME_TICKS = 300;

#define FRAME_TIMER_MODE ((1 << TVE_CSR_MON) | (1 << TVE_CSR_RUN))

/**
 * @brief Заливка прямоугольника однобайтовым образцом color в видеопамяти
 *
 * @param x_graph - координата X по которой будет осуществлён вывод прямоугольника
 * @param y_graph - координата Y по которой будет осуществлён вывод прямоугольника
 * @param x_width - ширина прямоугольника в байтах
 * @param y_width - высота прямоугольника в строках
 * @param color   - цвет прямоугольника в виде байта
 */
static void paint_brick(uint16_t x_graph, uint16_t y_graph, uint16_t x_width, uint16_t y_width, uint8_t color)
{
    volatile uint8_t *p = (volatile uint8_t *)MEM_VIDEO + y_graph * SCREEN_BYTE_WIDTH + x_graph;
    while (y_width--)
    {
        for (uint16_t i = 0; i < x_width; ++i) p[i] = color;
        p += SCREEN_BYTE_WIDTH;
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
        switch (c)
        {
            case ' ':
            {
                sp_put(x_graph, y_graph, sizeof(ch_space[0]), sizeof(ch_space) / sizeof(ch_space[0]), (uint8_t *)ch_space, nullptr); // Вывести спрайт пробела
                break;
            }

            case '0' ... '9':
            {
                c -= '0';
                sp_put(x_graph, y_graph, sizeof(ch_digits[0][0]), sizeof(ch_digits[0]) / sizeof(ch_digits[0][0]), (uint8_t *)ch_digits[c], nullptr); // Вывести спрайт цифры
                break;
            }

            case 'A' ... 'Z':
            {
                c -= 'A';
                sp_put(x_graph, y_graph, sizeof(ch_alpha[0][0]), sizeof(ch_alpha[0]) / sizeof(ch_alpha[0][0]), (uint8_t *)ch_alpha[c], nullptr); // Вывести спрайт буквы
                break;
            }

            case '.':
            {
                sp_put(x_graph, y_graph, sizeof(ch_dot[0]), sizeof(ch_dot) / sizeof(ch_dot[0]), (uint8_t *)ch_dot, nullptr); // Вывести спрайт точки
                break;
            }

            default:
            {
                sp_put(x_graph, y_graph, sizeof(ch_underline[0]), sizeof(ch_underline) / sizeof(ch_underline[0]), (uint8_t *)ch_underline, nullptr); // Вывести спрайт подчёркивания
            }
        }

        x_graph += sizeof(ch_alpha[0][0]);
    }
}

constexpr uint16_t char_width = sizeof(ch_alpha[0][0]);
constexpr uint16_t str_height = sizeof(ch_alpha[0]) / char_width;
constexpr uint16_t y_space = 6;
constexpr uint16_t windmill_height = 42;
constexpr uint16_t table_height = SCREEN_PIX_HEIGHT - (str_height + y_space) - windmill_height;

const char unpacking_str[] = "UNPACKING...";
constexpr uint16_t unpacking_str_x_pos = (SCREEN_BYTE_WIDTH - char_width * sizeof(unpacking_str) + char_width) / 2;
constexpr uint16_t unpacking_str_y_pos = (SCREEN_PIX_HEIGHT + str_height) / 2;

const char loading_str[] = "LOADING";
constexpr uint16_t loading_str_x_pos = (SCREEN_BYTE_WIDTH - char_width * sizeof(loading_str) + char_width) / 2;
constexpr uint16_t loading_str_y_pos = str_height + y_space + 2 + table_height + y_space;

static const char game_filename_str[sizeof(STR(BIN_FILE_1))] = STR(BIN_FILE_1);
constexpr uint16_t game_filename_x_pos = (SCREEN_BYTE_WIDTH - char_width * sizeof(game_filename_str) + char_width) / 2;
constexpr uint16_t game_filename_y_pos = loading_str_y_pos + str_height + y_space;

const char digger_str[] = "D I G G E R";
constexpr uint16_t digger_str_x_pos = (SCREEN_BYTE_WIDTH - char_width * sizeof(digger_str) + char_width) / 2;

const char one_str[] = "ONE";
constexpr uint16_t one_x_pos = (SCREEN_BYTE_WIDTH + SCREEN_BYTE_WIDTH / 2 - char_width * sizeof(one_str) + char_width) / 2;

const char player_str[] = "PLAYER";
constexpr uint16_t player_x_pos = (SCREEN_BYTE_WIDTH + SCREEN_BYTE_WIDTH / 2 - char_width * sizeof(player_str) + char_width) / 2;

uint16_t one_player_y;

/**
 * @brief Инициализация демо
 */
void init_demo()
{
    paint_brick(0, 0, SCREEN_BYTE_WIDTH, SCREEN_PIX_HEIGHT, 0); // Очистка экрана

    uint16_t y_pos = 0;

    // Строка "D I G G E R"
    print_str(digger_str, digger_str_x_pos, y_pos);
    y_pos += str_height + y_space;

    // Верхняя линия рамки
    paint_brick(0, y_pos, SCREEN_BYTE_WIDTH, 2, 0b01010101);
    y_pos += 2;

    one_player_y = y_pos + y_space;

    // Строка "ONE"
    print_str(one_str, one_x_pos, one_player_y);
    one_player_y += str_height + y_space;

    // Строка "PLAYER"
    print_str(player_str, player_x_pos, one_player_y);

    one_player_y += str_height + y_space * 2;

    // Вертикальные линии рамки и разделитель
    paint_brick(0, y_pos, 1, table_height, 0b00000001);
    paint_brick(SCREEN_BYTE_WIDTH - 1, y_pos, 1, table_height, 0b01000000);
    paint_brick(SCREEN_BYTE_WIDTH / 2, y_pos, 1, table_height, 0b01000000);
    paint_brick(SCREEN_BYTE_WIDTH / 2 + 1, y_pos, 1, table_height, 0b00000001);
    y_pos += table_height;

    // Нижняя линия рамки
    paint_brick(0, y_pos, SCREEN_BYTE_WIDTH, 2, 0b01010101);

    // Запустить кадровый таймер демо. Лимит выставляется один раз; режим
    // непрерывный (CAP=0, OS=0), счётчик автоматически перезагружается.
    volatile uint16_t      *lim = (volatile uint16_t *)REG_TVE_LIMIT;
    volatile union TVE_CSR *csr = (volatile union TVE_CSR *)REG_TVE_CSR;
    *lim = FRAME_TICKS;
    csr->reg = FRAME_TIMER_MODE;
}

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
 * @brief Обработка общего состояния демо
 */
void process_demo_state()
{
    constexpr uint16_t image_width = sizeof(image_nobbin[0][0]);
    constexpr uint16_t image_height = sizeof(image_nobbin[0]) / sizeof(image_nobbin[0][0]); // Высота спрайта Ноббина
    constexpr uint16_t y_space = 8;
    constexpr uint16_t move_start_pos = SCREEN_BYTE_WIDTH - 6;

    constexpr uint16_t start_time = 0;
    constexpr uint16_t start_delay = 128;
    constexpr uint16_t move_durance = 184;
    constexpr uint16_t end_to_print = 16;
    constexpr uint16_t print_to_next = 128;

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
    constexpr uint16_t bag_display_time = digger_print_time + move_durance;
    constexpr uint16_t bag_print_time = bag_display_time + end_to_print;

    // Тайминги отображения монеты в демо
    constexpr uint16_t emerald_display_time = bag_print_time + move_durance;
    constexpr uint16_t emerald_print_time = emerald_display_time + end_to_print;

    // Тайминги отображения вишенки в демо
    constexpr uint16_t cherry_display_time = emerald_print_time + move_durance;
    constexpr uint16_t cherry_print_time = cherry_display_time + end_to_print;

    // Время до повтора демо
    constexpr uint16_t demo_restart_time = cherry_print_time + 250;

    switch (demo_time)
    {
        case start_time:
        {
            // Очистка области Demo
            nobbin_x = hobbin_x = digger_x = 0;
            constexpr uint16_t demo_height = table_height - (str_height + y_space * 2) * 2;
            paint_brick(SCREEN_BYTE_WIDTH / 2 + 2, one_player_y, SCREEN_BYTE_WIDTH / 2 - 3, demo_height, 0);
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
            if (!(demo_time & 7)) nobbin_x--;
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
            if (!(demo_time & 7)) hobbin_x--;
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
            if (!(demo_time & 7)) digger_x--;
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
        paint_brick(nobbin_x + image_width, nobbin_y, 1, image_height, 0);
        sp_4_15_put(nobbin_x, nobbin_y, (uint8_t *)image_nobbin[image_phase]);
    }

    if (hobbin_x)
    {
        paint_brick(hobbin_x + image_width, hobbin_y, 1, image_height, 0);
        if (hobbin_mirror) sp_4_15_put(hobbin_x, hobbin_y, (uint8_t *)image_hobbin_left[image_phase]);
        else sp_4_15_put(hobbin_x, hobbin_y, (uint8_t *)image_hobbin_right[image_phase]);
    }

    if (digger_x)
    {
        paint_brick(digger_x + image_width, digger_y, 1, image_height, 0);
        if (digger_mirror) sp_4_15_put(digger_x, digger_y, (uint8_t *)image_digger_left[image_phase]);
        else sp_4_15_put(digger_x, digger_y, (uint8_t *)image_digger_right[image_phase]);
    }

    if (!(demo_time & 7))
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

    // Ограничение частоты кадров демо.
    // Ждём истечения таймера FRAME_TICKS, перезапускаем его (запись в CSR сбрасывает флаг FL).
    volatile union TVE_CSR *csr = (volatile union TVE_CSR *)REG_TVE_CSR;
    while ((csr->reg & (1 << TVE_CSR_FL)) == 0);
    csr->reg = FRAME_TIMER_MODE;
}

extern void start();

// Распаковщик ZX0
static const uint8_t *zx0_src;
static uint8_t zx0_bit_mask;  //< Число оставшихся в `zx0_bit_value` бит (8..1)
static uint8_t zx0_bit_value;
static uint8_t zx0_last_byte;
static uint8_t zx0_backtrack;

/*
    Активный бит всегда в позиции 7, поэтому возврат сводится к `value >> 7`
    (для uint8_t это уже 0/1) — обходим баг кодогенерации gcc -Os -mlra на
    идиоме `(a & b) ? 1 : 0`, где `neg`-флаг затирается следующим `clr` и
    функция всегда возвращает 0.
*/
static uint8_t zx0_read_bit()
{
    uint8_t bit;
    if (zx0_backtrack)
    {
        zx0_backtrack = 0;
        return zx0_last_byte & 1;
    }
    if (!zx0_bit_mask)
    {
        zx0_bit_value = *zx0_src++;
        zx0_bit_mask  = 8;
    }
    bit            = zx0_bit_value >> 7;
    zx0_bit_value  = zx0_bit_value << 1;
    zx0_bit_mask--;
    return bit;
}

static uint16_t zx0_elias(uint8_t inverted)
{
    uint16_t value = 1;
    while (!zx0_read_bit())
    {
        value = (value << 1) | (zx0_read_bit() ^ inverted);
    }
    return value;
}

/**
 * @brief Распаковка потока ZX0 в произвольную область памяти.
 *
 * @param src - указатель на сжатый поток ZX0
 * @param dst - указатель на буфер-приёмник; он же служит «историей» для ссылок по смещению
 */
void zx0_decompress(const uint8_t *src, uint8_t *dst)
{
    uint16_t last_offset = 1;
    uint16_t length;
    uint8_t *p;
    uint8_t bit;

    zx0_src       = src;
    zx0_bit_mask  = 0;
    zx0_backtrack = 0;

    for (;;)
    {
        /* === LITERALS === */
        length = zx0_elias(0);
        while (length--) *dst++ = *zx0_src++;
        bit = zx0_read_bit();

        if (!bit)
        {
            /* === COPY_FROM_LAST_OFFSET === */
            length = zx0_elias(0);
            p = dst - last_offset;
            while (length--) *dst++ = *p++;
            bit = zx0_read_bit();
        }

        while (bit)
        {
            /* === COPY_FROM_NEW_OFFSET === */
            uint16_t hi = zx0_elias(1);
            if (hi == 256) return;             // Маркер конца потока

            zx0_last_byte = *zx0_src++;
            last_offset   = (hi << 7) - (zx0_last_byte >> 1);
            zx0_backtrack = 1;                 // Бит 0 только что прочитанного байта — первый бит следующей гаммы
            length = zx0_elias(0) + 1;
            p = dst - last_offset;
            while (length--) *dst++ = *p++;
            bit = zx0_read_bit();
        }
    }
}

/**
 * @brief Проверка нажатия любой клавиши клавиатуры или кнопки джойстика
 */
static bool any_key_or_button_pressed()
{
    volatile union KEY_STATE *key_state_ptr = (volatile union KEY_STATE *)REG_KEY_STATE;
    volatile uint16_t        *par_port_ptr = (volatile uint16_t *)REG_PAR_INTERF;

    if (key_state_ptr->bits.STATE) return true;
    return (*par_port_ptr & ((1 << PAR_INTERF_LEFT_BUTTON) | (1 << PAR_INTERF_RIGHT_BUTTON))) != 0;
}

/**
 * @brief Проигрывание ноты с амплитудной огибающей
 *
 * Однобитовый динамик БК позволяет менять громкость только через PWM:
 * за один аудио-цикл (2*period sob-тактов) включаем динамик на `pw` тактов, выключаем на остальные.
 * Когда pw близок к period - 50% duty, максимальный звук; при малых pw - почти тишина.
 *
 * Форма огибающей - мгновенная атака, длинный hold на полной громкости, затем плавный спад к тишине.
 * Скважность PWM: 3/8 длительности - полный звук, далее 4 убывающих стадии по 1/8 со ступенчатым
 * делением PW пополам. Нота играет 7/8 от выделенного durance.
 * Небольшая пауза в конце позволяет точно подогнать темп под оригинал.
 *
 * Цикл по стадиям decay написан так, чтобы PW каждой стадии получался
 * единым сдвигом локального регистра.
 */
static void play_note_env(uint16_t period, uint16_t durance)
{
    if (durance < 8)
    {
        // Для нот короче 8 полупериодов огибающая не помещается откатываемся на плоский PWM в полную громкость
        sound_pwm(period, durance, period);
        return;
    }

    const uint16_t base = durance >> 3;            // 1/8 длительности
    sound_pwm(period, (base << 1) + base, period); // hold 3/8 на полной громкости

    uint16_t pw = period;
    for (uint8_t s = 0; s < 4; ++s)
    {
        pw >>= 1;  // 1/2, 1/4, 1/8, 1/16 от периода
        sound_pwm(period, base, pw);
    }
}

// Проигрыватель музыки Popcorn на заставке
static void play_popcorn()
{
    for (;;)
    {
        for (uint16_t i = 0; popcorn_periods[i] != 0; ++i)
        {
            if (any_key_or_button_pressed())
            {
                (void)*(volatile uint16_t *)REG_KEY_DATA; // Очистка буфера клавиатуры
                return;
            }

            const uint16_t p = popcorn_periods[i];
            play_note_env(p + (p >> 5), popcorn_durations[i]);
        }
    }
}

// Загрузка и запуск основного файла игры
static void load_and_run_digger(void) __attribute__((noreturn));
static void load_and_run_digger(void)
{
    // Показать строку "LOADING"
    print_str(loading_str, loading_str_x_pos, loading_str_y_pos);

    // Показать строку с именем загружаемого файла
    print_str(game_filename_str, game_filename_x_pos, game_filename_y_pos);

    // Подготовить блок параметров драйвера магнитофона в системной области
    struct EMT_36_PARAMS *p = (struct EMT_36_PARAMS *)SYS_EMT_36_PARAMS;
    p->COMMAND  = EMT_36_FILE_READ;   // Считывание файла через EMT36
    p->DATA_PTR = (uint8_t *)nullptr; // Использовать адрес загрузки из заголовка файла
    p->SIZE     = 0;                  // Размер из заголовка, без ограничения (0)
    for (uint8_t i = 0; i < 16; i++)  // Фромирование 16 байт имени файлв
    {
        if (i < sizeof(game_filename_str)) p->NAME[i] = game_filename_str[i]; // Копирование из статического поля с именем
        else p->NAME[i] = ' '; // Дополнение до 16 байт нулями
    }

    // Загрузить файл с магнитофона через вызов EMT36
    EMT_36((const char *)p);

    // Перейти на entry-point загруженного файла игры
    asm volatile ("jmp @#01000\n" ::: "memory");
    __builtin_unreachable();
}

/**
 * @brief Основная программа
 */
void main()
{
    paint_brick(0, 0, SCREEN_BYTE_WIDTH, SCREEN_PIX_HEIGHT, 0); // Очистка экрана

    set_PSW(1 << PSW_I); // Замаскировать прерывания IRQ
    ((union KEY_STATE *)REG_KEY_STATE)->bits.INT_MASK = 1; // Отключить прерывание от клавиатуры

    // Отображение строки "UNPACKING..."
    print_str(unpacking_str, unpacking_str_x_pos, unpacking_str_y_pos);

    // Распаковать заставку в экранное ОЗУ
    zx0_decompress(cover_zx0, (uint8_t *)MEM_VIDEO);

    // Воспроизвести музыку Popcorn
    play_popcorn();

    init_demo(); // Инициализация демо
    for (;;)
    {
        process_demo_state(); // Обработка состояний демо
        if (any_key_or_button_pressed()) load_and_run_digger(); // Запустить игру при нажатии клавиши или кнопки джойстика
    }
}
