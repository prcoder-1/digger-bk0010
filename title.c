#include "memory.h"
#include "sprites_title.h"
#include "sound.h"
#include "tools.h"
#include "emt.h"
#include "digger_sprites_title.h"
#include "digger_music_title.h"
#include "digger_full_font.h"

#define COIN_Y_OFFSET 3 // Смещение спрайта монетки в ячейке по оси Y

#define SND_TIMER_MODE         ((1 << TVE_CSR_MON) | (1 << TVE_CSR_RUN)) // без предделителей: 1 такт = 1/23438 c
#define SND_GAP_CYCLES         64u
#define SND_END_PAUSE_CYCLES   (2 * 23438u)

uint16_t snd_note_idx       = 0; // индекс текущей ноты в popcorn_periods/durations
uint16_t snd_period         = 1; // средний полупериод текущей ноты в тактах таймера
uint16_t snd_cycles_left    = 0; // тактов до конца текущей ноты
uint16_t snd_cycles_total   = 0; // полная длительность текущей ноты (для расчёта огибающей)
uint16_t snd_silence_cycles = 0; // тактов до конца межнотной паузы
uint16_t snd_speaker        = 0; // текущее состояние бита динамика (0 или 0100)
uint16_t snd_frame_ticks    = 0; // абсолютная wall-time-метка в тактах для пейсинга кадров демо

// Полу-период синуса в 17 точках, нормировано к 64 = sin(π/2). Используется
// для построения огибающей по форме sin(π·t) от 0 до 1 и обратно до 0.
static const uint8_t snd_sin_table[17] = {
    0, 12, 24, 36, 45, 53, 59, 63, 64, 63, 59, 53, 45, 36, 24, 12, 0
};

// Таблица on_ticks (в тактах таймера) для огибающей текущей ноты.
// snd_on_dur_ticks[i] = snd_sin_table[i] · snd_period / 128, что даёт максимум
// скважности 25 % (на пике sin = 64, on_ticks = snd_period / 2 при полном
// периоде 2·snd_period).
static uint16_t snd_on_dur_ticks[17];

// Соответствующее количество итераций sob в CPU-loop'е для выдержки ON-полуволны.
// 1 такт таймера = 128 CPU-циклов (при f/128). sob занимает ~6 циклов,
// поэтому 1 такт ≈ 21 итерация. Множитель можно подстроить, если на реальном
// железе sob идёт быстрее/медленнее.
static uint16_t snd_on_dur_iters[17];
#define SND_DELAY_ITERS_PER_TICK 10u

/**
 * @brief Сервис одного события таймера.
 *
 * Горячий путь (активная нота продолжается) идёт первой ветвью без
 * предварительных операций над глобалами — щелчок и рестарт таймера
 * выполняются в первые ~10 инструкций после входа, чтобы джиттер
 * полупериода зависел только от полленг-задержки на стороне вызова.
 *
 * На смене ноты — ровно один csr->reg = SND_TIMER_MODE с уже новым
 * LIMIT (раньше было два: первый в начале функции с *старым* LIMIT,
 * через ~80 циклов второй с новым; это давало одну заметно
 * растянутую полуволну в момент каждой смены ноты).
 */
void music_service()
{
    volatile union TVE_CSR *csr = (volatile union TVE_CSR *)REG_TVE_CSR;
    volatile uint16_t      *lim = (volatile uint16_t *)REG_TVE_LIMIT;
    volatile uint16_t      *spk = (volatile uint16_t *)REG_EXT_DEV;

    // === HOT PATH: активная нота, один сервис = одна полная фаза меандра ===
    // ON-полуволна тактуется CPU-loop'ом (точно on_ticks тактов таймера, без
    // зависимости от полленг-задержки). OFF-полуволна — таймером: LIMIT
    // выставляется в off_ticks, и следующий FL придёт строго через off_ticks.
    if (snd_cycles_left > (snd_period << 1))
    {
        uint16_t progress = snd_cycles_total - snd_cycles_left;
        uint16_t env_raw  = (progress < snd_cycles_left) ? progress : snd_cycles_left;
        uint16_t idx      = env_raw >> 7;
        // Клампим на пике (idx=8), а не на конце LUT (idx=16). snd_sin_table
        // симметричная: [0]=0, [8]=64 (пик), [16]=0. env_raw сам по себе уже
        // пилообразный (0→max→0 за длительность ноты), и если пускать idx
        // дальше пика в LUT, в середине ноты получаются ДВА пика с провалом
        // (тишиной) между ними. Клампя на 8, после атаки получаем плато на
        // полной громкости до тех пор, пока env_raw не начнёт убывать.
        if (idx > 8) idx = 8;
        uint16_t on_ticks    = snd_on_dur_ticks[idx];
        uint16_t delay_count = snd_on_dur_iters[idx];
        uint16_t off_ticks   = (snd_period << 1) - on_ticks;
        if (off_ticks < 1) off_ticks = 1;

        *spk = 0100;                       // динамик ON
        if (delay_count) {
            asm volatile (
                "mov %0, r0\n\t"
                "1: sob r0, 1b\n\t"
                : : "r"(delay_count) : "r0", "cc"
            );
        }
        *spk = 0;                          // динамик OFF
        *lim = off_ticks;
        csr->reg = SND_TIMER_MODE;          // рестарт таймера на off_ticks

        snd_cycles_left -= snd_period << 1;
        snd_frame_ticks += snd_period << 1;
        return;
    }

    snd_frame_ticks += snd_period << 1;     // учли только что прошедший период

    // === Активная нота кончается на этом тике ===
    if (snd_cycles_left)
    {
        snd_speaker        = 0;
        *spk               = 0;             // погасить динамик одной записью
        // Восстанавливаем LIMIT = snd_period для silence-фазы.
        *lim               = snd_period;
        csr->reg           = SND_TIMER_MODE;
        snd_cycles_left    = 0;
        snd_silence_cycles = SND_GAP_CYCLES;
        return;
    }

    // === Межнотная пауза ===
    if (snd_silence_cycles > snd_period)
    {
        csr->reg            = SND_TIMER_MODE;
        snd_silence_cycles -= snd_period;
        return;
    }
    snd_silence_cycles = 0;

    // === Загрузка следующей ноты ===
    uint8_t next_period = popcorn_periods[snd_note_idx];
    if (!next_period)
    {
        // Конец мелодии — длинная пауза, потом сначала
        snd_note_idx       = 0;
        snd_silence_cycles = SND_END_PAUSE_CYCLES;
        csr->reg           = SND_TIMER_MODE;
        return;
    }

    uint8_t duration = popcorn_durations[snd_note_idx];
    snd_note_idx++;
    // Промежуточный темп: duration × 1472 = duration × (1024 + 512 − 64).
    // 71.9 % от исходного << 11. Три сдвига + сложение + вычитание.
    snd_cycles_left  = ((uint16_t)duration << 10) + ((uint16_t)duration << 9) - ((uint16_t)duration << 6);
    snd_cycles_total = snd_cycles_left;             // полная длительность для огибающей
    snd_period       = next_period;

    // Предвычисляем огибающую:
    //   on_ticks  — длительность ON-полуволны в тактах таймера (для off_ticks).
    //   on_iters  — количество итераций sob в CPU-loop для выдержки ON-полуволны
    //              ровно on_ticks тактов. 1 такт ≈ 21 итерация sob.
    //
    // Сдвиг >> 8 даёт максимум скважности 12.5 % (на пике sin=64, on_ticks =
    // snd_period / 4 при полном периоде 2·snd_period). Узкие импульсы дают
    // более мягкий тон с меньшим количеством высоких гармоник.
    for (uint8_t i = 0; i < 17; i++) {
        uint16_t on_ticks   = ((uint16_t)snd_sin_table[i] * (uint16_t)next_period) >> 8;
        snd_on_dur_ticks[i] = on_ticks;
        snd_on_dur_iters[i] = on_ticks * SND_DELAY_ITERS_PER_TICK;
    }

    // Запускаем таймер на полный период (2·snd_period). Первый FL придёт через
    // полный период, тогда стартует первый сервис ноты.
    *lim     = (uint16_t)next_period << 1;
    csr->reg = SND_TIMER_MODE;
}

/**
 * @brief Инлайновая проверка FL и вызов music_service.
 *
 * Используется как макрос на всех вызывающих сайтах в title.c, чтобы убрать
 * jsr+rts накладные (~15 циклов) на каждый поллинг. Это и сокращает общую
 * нагрузку на CPU за кадр (~30 мкс с шести межблочных вызовов плюс
 * сотни мкс с финального polling-loop), и зажимает максимальный
 * полленг-интервал — отчего напрямую уменьшается дрожание частоты нот.
 */
#define MUSIC_TICK() do { \
    asm volatile ( \
        "tstb @%[csr]\n\t" \
        "bpl 1f\n\t" \
        "jsr pc, _music_service\n" \
        "1:\n\t" \
        : : [csr]"i"(REG_TVE_CSR) : "r0", "r1", "cc", "memory" \
    ); \
} while (0)

/**
 * @brief Вызов тика музыки (внешний символ для asm-блоков, оставлен для совместимости)
 */
__attribute__((noinline)) void music_tick()
{
    asm volatile (
        "tstb @%[csr]\n\t"     // FL — старший бит CSR
        "bpl .l_skip_%=\n\t"
        "jsr pc, _music_service\n"
".l_skip_%=:\n\t"
        :
        : [csr]"i"(REG_TVE_CSR)
        : "r0", "r1", "cc", "memory"
    );
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
            title_sp_put(x_graph, y_graph, sizeof(ch_alpha[0][0]), sizeof(ch_alpha[0]) / sizeof(ch_alpha[0][0]), (uint8_t *)ch_alpha[index], nullptr); // Вывести спрайт буквы
        }

        x_graph += sizeof(ch_alpha[0][0]);
        MUSIC_TICK();
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

    volatile union TVE_CSR *csr = (volatile union TVE_CSR *)REG_TVE_CSR;
    volatile uint16_t      *lim = (volatile uint16_t *)REG_TVE_LIMIT;

    *lim = 0;
    csr->reg = SND_TIMER_MODE;
}

// Длительность одного «кадра» демо в тактах таймера (23438 Гц).
constexpr uint16_t FRAME_TICKS = 200;

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
    constexpr uint16_t move_durance = 184; // 176
    constexpr uint16_t end_to_print = 16;
    constexpr uint16_t print_to_next = 16;

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
    constexpr uint16_t demo_restart_time = cherry_print_time + 256;

    uint16_t frame_target = snd_frame_ticks + FRAME_TICKS;

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
    MUSIC_TICK();

    if (hobbin_x)
    {
        title_sp_clear_strip(hobbin_x + image_width, hobbin_y, image_height);
        if (hobbin_mirror) title_sp_4_15_put(hobbin_x, hobbin_y, (uint8_t *)image_hobbin_left[image_phase]);
        else title_sp_4_15_put(hobbin_x, hobbin_y, (uint8_t *)image_hobbin_right[image_phase]);
    }
    MUSIC_TICK();

    if (digger_x)
    {
        title_sp_clear_strip(digger_x + image_width, digger_y, image_height);
        if (digger_mirror) title_sp_4_15_put(digger_x, digger_y, (uint8_t *)image_digger_left[image_phase]);
        else title_sp_4_15_put(digger_x, digger_y, (uint8_t *)image_digger_right[image_phase]);
    }
    MUSIC_TICK();

    // Компенсация за отсутствующие анимационные спрайты. Один реальный
    // спрайт (clear_strip + sp_4_15_put) — это примерно 30 поллингов с
    // интервалом ~30 циклов между ними. Чтобы темп и распределение
    // полленг-задержки L не зависели от количества спрайтов на экране,
    // выполняем эквивалентное количество "пустых" поллингов за каждый
    // недостающий спрайт.
    {
        uint8_t missing_sprites = 0;
        if (!nobbin_x) missing_sprites++;
        if (!hobbin_x) missing_sprites++;
        if (!digger_x) missing_sprites++;

        while (missing_sprites--)
        {
            // 30 поллингов с задержкой ~20 циклов между ними — эквивалент
            // примерно одного цикла рисования спрайта (clear_strip 15 строк +
            // sp_4_15_put 15 строк × 2 поллинга/строка = ~45 поллингов, но
            // часть их короче — берём усреднённо ~30).
            uint8_t i = 30;
            while (i--)
            {
                MUSIC_TICK();
                asm volatile (
                    "nop\n\tnop\n\tnop\n\tnop\n\t"
                    "nop\n\tnop\n\tnop\n\tnop"
                );
            }
        }
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

    // Подгонка интервала между MUSIC_TICK в этом цикле к интервалам внутри
    // спрайтовых рутин (~25-30 циклов). Без этого polling-loop поллит таймер
    // плотнее, чем рисование, и средняя полленг-задержка отличается между
    // фазой "только polling-loop" (нет спрайтов на экране) и фазой "рисование +
    // polling-loop" (есть спрайты), что проявляется как разница в темпе и
    // дрожании звука.
    while ((int16_t)(snd_frame_ticks - frame_target) < 0)
    {
        MUSIC_TICK();
        asm volatile (
            "nop\n\tnop\n\tnop\n\tnop\n\t"
            "nop\n\tnop\n\tnop\n\tnop\n\t"
            "nop\n\tnop\n\tnop\n\tnop"
        );
    }
}

extern void start();

/**
 * @brief Основная программа
 */
void main()
{
    // EMT_14();

    typedef void (*vector)();
    *((volatile vector *)VEC_STOP) = start; // Установить вектор клавиши "СТОП" на _start

    // EMT_16(0233);
    // EMT_16(0236);

    set_PSW(1 << PSW_I); // Замаскировать прерывания IRQ
    ((union KEY_STATE *)REG_KEY_STATE)->bits.INT_MASK = 1; // Отключить прерывание от клавиатуры

    init_demo(); // Инициализация демо
    for (;;) process_demo_state(); // Основной бесконечный цикл демо
}
