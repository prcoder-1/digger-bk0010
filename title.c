#include "memory.h"
#include "sprites_title.h"
#include "sound.h"
#include "tools.h"
#include "emt.h"
#include "digger_sprites_title.h"
#include "digger_music_title.h"
#include "digger_full_font.h"
#include "digger_title.h"

#define COIN_Y_OFFSET 3 // Смещение спрайта монетки в ячейке по оси Y

// Длительность одного «кадра» демо в тактах таймера (23438 Гц).
// 150 тактов ≈ 6.4 мс/кадр, тот же бюджет, что был в исходной версии
// с поллинг-музыкой (там этот же объём времени уходил на music_service).
constexpr uint16_t FRAME_TICKS = 150;

#define FRAME_TIMER_MODE ((1 << TVE_CSR_MON) | (1 << TVE_CSR_RUN))

/**
 * @brief Заглушка для music-aware блиттеров из sprites_title.c.
 *
 * Эти блиттеры (title_sp_4_15_put и пр.) предназначались для проигрывания
 * музыки во время отрисовки и в каждой итерации зовут music_service. В
 * текущей сборке музыка играется синхронно ДО демо (см. play_popcorn),
 * поэтому music_service - пустая функция: jsr+rts ≈ 16 циклов на итерацию
 * блиттера, на размер титульного экрана это пренебрежимо.
 */
void music_service() { }

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
            if (c == '.')
            {
                title_sp_put(x_graph, y_graph, sizeof(ch_dot[0]), sizeof(ch_dot) / sizeof(ch_dot[0]), (uint8_t *)ch_dot, nullptr); // Вывести спрайт точки
            }
            else
            {
                uint16_t index = c - 'A';
                title_sp_put(x_graph, y_graph, sizeof(ch_alpha[0][0]), sizeof(ch_alpha[0]) / sizeof(ch_alpha[0][0]), (uint8_t *)ch_alpha[index], nullptr); // Вывести спрайт буквы
            }
        }

        x_graph += sizeof(ch_alpha[0][0]);
    }
}

constexpr uint16_t char_width = sizeof(ch_alpha[0][0]);
constexpr uint16_t str_height = sizeof(ch_alpha[0]) / char_width;
constexpr uint16_t y_space = 8;
constexpr uint16_t windmill_height = 42;
constexpr uint16_t table_height = SCREEN_PIX_HEIGHT - (str_height + y_space) - windmill_height;

const char unpacking_str[] = "UNPACKING...";
constexpr uint16_t unpacking_str_x_pos = (SCREEN_BYTE_WIDTH - char_width * sizeof(unpacking_str) + char_width) / 2;
constexpr uint16_t unpacking_str_y_pos = (SCREEN_PIX_HEIGHT + str_height) / 2;

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
    title_sp_paint_brick_long(0, 0, SCREEN_BYTE_WIDTH, SCREEN_PIX_HEIGHT, 0); // Очистка экрана

    uint16_t y_pos = 0;

    // Строка "D I G G E R"
    print_str(digger_str, digger_str_x_pos, y_pos);
    y_pos += str_height + y_space;

    // Верхняя линия рамки
    title_sp_paint_brick_long(0, y_pos, SCREEN_BYTE_WIDTH, 2, 0b01010101);
    y_pos += 2;

    one_player_y = y_pos + y_space;

    // Строка "ONE"
    print_str(one_str, one_x_pos, one_player_y);
    one_player_y += str_height + y_space;

    // Строка "PLAYER"
    print_str(player_str, player_x_pos, one_player_y);

    one_player_y += str_height + y_space * 2;

    // Вертикальные линии рамки и разделитель
    title_sp_paint_brick_long(0, y_pos, 1, table_height, 0b00000001);
    title_sp_paint_brick_long(SCREEN_BYTE_WIDTH - 1, y_pos, 1, table_height, 0b01000000);
    title_sp_paint_brick_long(SCREEN_BYTE_WIDTH / 2, y_pos, 1, table_height, 0b01000000);
    title_sp_paint_brick_long(SCREEN_BYTE_WIDTH / 2 + 1, y_pos, 1, table_height, 0b00000001);
    y_pos += table_height;

    // Нижняя линия рамки
    title_sp_paint_brick_long(0, y_pos, SCREEN_BYTE_WIDTH, 2, 0b01010101);

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
            title_sp_paint_brick_long(SCREEN_BYTE_WIDTH / 2 + 2, one_player_y, SCREEN_BYTE_WIDTH / 2 - 3, demo_height, 0);
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
            title_sp_4_15_put(bag_x, bag_y, (uint8_t *)image_bag);
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
            title_sp_put(emerald_x, emerald_y + COIN_Y_OFFSET, sizeof(image_coin[0]), sizeof(image_coin) / sizeof(image_coin[0]), (uint8_t *)image_coin, nullptr);
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
            title_sp_4_15_put(cherry_x, cherry_y, (uint8_t *)image_cherry);
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
        title_sp_clear_strip(nobbin_x + image_width, nobbin_y, image_height);
        title_sp_4_15_put(nobbin_x, nobbin_y, (uint8_t *)image_nobbin[image_phase]);
    }

    if (hobbin_x)
    {
        title_sp_clear_strip(hobbin_x + image_width, hobbin_y, image_height);
        if (hobbin_mirror) title_sp_4_15_put(hobbin_x, hobbin_y, (uint8_t *)image_hobbin_left[image_phase]);
        else title_sp_4_15_put(hobbin_x, hobbin_y, (uint8_t *)image_hobbin_right[image_phase]);
    }

    if (digger_x)
    {
        title_sp_clear_strip(digger_x + image_width, digger_y, image_height);
        if (digger_mirror) title_sp_4_15_put(digger_x, digger_y, (uint8_t *)image_digger_left[image_phase]);
        else title_sp_4_15_put(digger_x, digger_y, (uint8_t *)image_digger_right[image_phase]);
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

    // Кадровый пейсинг: ждём истечения таймера FRAME_TICKS, перезапускаем
    // его (запись в CSR сбрасывает флаг FL). Без звука это единственный
    // ограничитель темпа демо.
    volatile union TVE_CSR *csr = (volatile union TVE_CSR *)REG_TVE_CSR;
    while ((csr->reg & (1 << TVE_CSR_FL)) == 0);
    csr->reg = FRAME_TIMER_MODE;
}

extern void start();

// === Распаковщик ZX0 (modern v2, прямой поток) ============================
// Состояние в файловой области — компактнее, чем гонять указатели через стек.
static const uint8_t *zx0_src;
static uint8_t zx0_bit_mask;
static uint8_t zx0_bit_value;
static uint8_t zx0_last_byte;
static uint8_t zx0_backtrack;

// `zx0_bit_mask` хранит число оставшихся в `zx0_bit_value` бит (8..1).
// Активный бит всегда в позиции 7, поэтому возврат сводится к `value >> 7`
// (для uint8_t это уже 0/1) — обходим баг кодогенерации gcc -Os -mlra на
// идиоме `(a & b) ? 1 : 0`, где `neg`-флаг затирается следующим `clr` и
// функция всегда возвращает 0.
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
            if (hi == 256) return;             // маркер конца потока

            zx0_last_byte = *zx0_src++;
            last_offset   = (hi << 7) - (zx0_last_byte >> 1);
            zx0_backtrack = 1;                 // бит 0 только что прочитанного байта
                                               // — первый бит следующей гаммы
            length = zx0_elias(0) + 1;
            p = dst - last_offset;
            while (length--) *dst++ = *p++;
            bit = zx0_read_bit();
        }
    }
}

/**
 * @brief Синхронное проигрывание мелодии «Popcorn» через sound_vibrato.
 *
 * Аналог проигрывания траурного марша в digger.c::man_rip(): идём по
 * параллельным массивам periods/durations, на каждой ноте sound_vibrato
 * блокирует CPU на её длительности. Терминатор - нулевой period.
 *
 * Между нотами опрашиваем регистр клавиатуры: любое нажатие прерывает
 * музыку, гасит флаг STATE и выходит из функции - так пользователь может
 * пропустить заставку, не дожидаясь конца мелодии.
 *
 * --- Соответствие PC-версии по строю ---
 * sound_vibrato выдаёт пульс с периодом волны 2*period*sob_cycles тактов
 * CPU (sob ~ 6 циклов на КР1801ВМ1, см. CLAUDE.md). При N=20 и period~228
 * для C4 это даёт ~1098 Гц = C6, на ДВЕ ОКТАВЫ выше PC (там 261.6 Гц).
 *
 * Полное соответствие PC требовало бы scale=4.78. На октаву ВЫШЕ PC
 * (что и хочется здесь) - scale = 4.78/2 = 2.39. Берём 2.0625 = 2+1/16,
 * реализуется как (p<<1)+(p>>4). Это даёт промах ~-100 центов от
 * (PC+октава) - значит звучит примерно на октаву выше PC (а не на две
 * октавы как было без скейла). Durance делим на 2 (>>1) - время ноты
 * сохраняется ~неизменным.
 *
 * Сдвиг применяется только в этом плейере: оставляет digger.c::man_rip
 * с прежним строем (пользователь к нему привык, переделка digger.c
 * массивов под uint16_t - отдельная задача).
 */
static void play_popcorn()
{
    volatile union KEY_STATE *ks = (volatile union KEY_STATE *)REG_KEY_STATE;
    volatile uint16_t        *kd = (volatile uint16_t *)REG_KEY_DATA;

    for (uint16_t i = 0; popcorn_periods[i] != 0; ++i)
    {
        if (ks->bits.STATE)
        {
            (void)*kd; // сбросить флаг STATE, чтобы дальше не сработал ложный wait
            return;
        }
        // Период домножаем на ~2.0625 ((p<<1)+(p>>4)) - сдвигает строй
        // вниз на одну октаву (между BK-without-scale и PC-частотой).
        // Durance делим на 2 - время звучания почти не меняется.
        const uint16_t p = popcorn_periods[i];
        sound_vibrato((p << 1) + (p >> 4), popcorn_durations[i] >> 1);
    }
}

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

    // Строка "UNPACKING..."
    print_str(unpacking_str, unpacking_str_x_pos, unpacking_str_y_pos);

    // Распаковать обложку прямо в экранное ОЗУ.
    zx0_decompress(cover_zx0, (uint8_t *)MEM_VIDEO);

    // Сразу после splash начать проигрывание Popcorn. Музыка прерывается
    // нажатием любой клавиши (см. play_popcorn). После окончания мелодии
    // или нажатия клавиши - переход к демо.
    play_popcorn();

    init_demo(); // Инициализация демо
    for (;;) process_demo_state(); // Основной бесконечный цикл демо
}
