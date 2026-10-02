#include "memory.h"
#include "sprites.h"
#include "sound.h"
#include "tools.h"
#include "emt.h"
#include "digger_sprites.h"
#include "digger_short_font.h"
#include "digger_levels.h"
#include "digger_music.h"
#include "digger_music_background.h"

constexpr uint8_t POS_X_STEP = 4;      // Шаг клеток по оси X (в байтах)
constexpr uint8_t POS_Y_STEP = 16;     // Шаг клеток по оси Y (в строках)
constexpr uint8_t MOVE_X_STEP = 1;     // Шаг перемещения по оси X (в байтах)
constexpr uint8_t MOVE_Y_STEP = 4;     // Шаг перемещения по оси Y (в строках)

constexpr uint16_t SCREEN_Y_OFFSET = 25;

constexpr uint16_t FIELD_X_OFFSET = 2;  // Смещение игрового поля по оси X
constexpr uint16_t FIELD_Y_OFFSET = SCREEN_Y_OFFSET + 32; // Смещение игрового поля по оси Y

constexpr uint16_t MIN_X_POS = FIELD_X_OFFSET; // Минимальное положение по оси X
constexpr uint16_t MIN_Y_POS = FIELD_Y_OFFSET; // Минимальное положение по оси Y
constexpr uint16_t MAX_X_POS = FIELD_X_OFFSET + POS_X_STEP * (W_MAX - 1); // Максимальное положение по оси X
constexpr uint16_t MAX_Y_POS = FIELD_Y_OFFSET + POS_Y_STEP * (H_MAX - 1); // Максимальное положение по оси Y

constexpr uint16_t COIN_Y_OFFSET = 3; // Смещение спрайта монетки в ячейке по оси Y

// Правый верхний угол игрового поля: там появляется вишенка-бонус и рождаются враги
constexpr uint8_t CORNER_X = FIELD_X_OFFSET + (W_MAX - 1) * POS_X_STEP;
constexpr uint8_t CORNER_Y = FIELD_Y_OFFSET;

constexpr uint8_t MAX_BAGS = 7; // Максимальное количество мешков с деньгами на уровне
constexpr uint8_t MAX_BUGS = 5; // Максимальное количество врагов на уровне
constexpr uint8_t MAX_LIVES = 6; // Максимальное количество жизней

constexpr uint8_t MAN_START_X = 7; // Начальное положение Диггера по оси X (в клетках)
constexpr uint8_t MAN_START_Y = 9; // Начальное положение Диггера по оси Y (в клетках)

// Кнопки джойстика: обе работают как огонь и как «любая кнопка»
constexpr uint16_t JOY_BUTTONS = (1 << PAR_INTERF_LEFT_BUTTON) | (1 << PAR_INTERF_RIGHT_BUTTON);

constexpr uint8_t LOOSE_WAIT = 15; // Время с момента начала покачивания до момента падения мешка

constexpr uint32_t BONUS_LIFE_SCORE = 20000; // Количество очков для дополнительной жизни

constexpr uint8_t SCORE_DIGITS = 6; // Количество выводимых десятичных знаков счёта (как в оригинале)
constexpr uint32_t MAX_SCORE = 999999UL; // Потолок счёта: при превышении счёт обнуляется (как в оригинале)

/**
 * @brief Перечисление типов врагов
 */
enum bug_types : uint8_t
{
    BUG_HOBBIN = 0, /**< Хоббин */
    BUG_NOBBIN      /**< Ноббин */
};

/**
 * @brief Перечисление направлений движения
 */
enum direction : uint8_t
{
    DIR_LEFT = 0, /**< Движется налево */
    DIR_RIGHT,    /**< Движется направо */
    DIR_UP,       /**< Движется вверх */
    DIR_DOWN,     /**< Движется вниз */
    DIR_STOP      /**< Стоит на месте */
};

/**
 * @brief Перечисление состояний Диггера или врага
 */
enum creature_state : uint8_t
{
    CREATURE_INACTIVE = 0,   /**< Не активен */
    CREATURE_STARTING,       /**< Стратует */
    CREATURE_ALIVE,          /**< Жив */
    CREATURE_DEAD_MONEY_BAG, /**< Убит мешком с деньгами */
    CREATURE_RIP             /**< Лежит дохлый */
};

enum bag_state : uint8_t
{
    BAG_INACTIVE = 0, /**< Мешок неактивен */
    BAG_STATIONARY,   /**< Мешок стационарен (стоит на месте) */
    BAG_LOOSE,        /**< Мешок раскачивается */
    BAG_FALLING,      /**< Мешок падает */
    BAG_BREAKS,       /**< Мешок разбивается */
    BAG_BROKEN        /**< Мешок разбился */
};

/**
 * @brief Перечисление состояний бонус-режима
 */
enum bonus_state : uint8_t
{
    BONUS_OFF = 0, /**< Режим бонус ещё не включен */
    BONUS_READY,   /**< Режим бонус готов к активации (появилась вишенка) */
    BONUS_ON,      /**< Режим бонус включен */
    BONUS_END      /**< Режим бонус закончился */
};

/**
 * @brief Состояние мешка с деньгами
 *
 * Размер дополнен до 8 байт: при индексации &bags_state[i] компилятор использует
 * сдвиг (<<3) вместо вызова __mulhi3 для умножения на 5.
 *
 * Координаты - слова, а не байты: они уходят в функции вывода спрайтов, которые
 * принимают uint16_t, и байтовое поле каждый раз требовало бы расширения до слова
 * парой clr/bisb. Порядок полей подобран так, чтобы слова легли на чётные смещения.
 */
struct bag_info
{
    enum bag_state state; ///< Флаг активности мешка
    enum direction dir;   ///< Направление движения мешка
    uint16_t x_graph;     ///< Положение по оси X в графических координатах
    uint16_t y_graph;     ///< Положение по оси Y в графических координатах
    uint8_t count;        ///< Счётчик
    uint8_t _pad[1];      ///< Выравнивание до 8 байт (см. комментарий выше)
};

/**
 * @brief Состояние врага (Хоббина/Ноббина)
 *
 * Размер дополнен до 16 байт по той же причине, что и bag_info.
 */
struct bug_info
{
    enum creature_state state; ///< Состояние врага (жив, погиб, лежит дохлый - влияет на внешний вид)
    enum bug_types type;       ///< Тип врага (Ноббин или Хоббин)
    uint16_t x_graph;          ///< Положение по оси X в графических координатах
    uint16_t y_graph;          ///< Положение по оси Y в графических координатах
    enum direction dir;        ///< Направление движения врага
    uint8_t count;             ///< Счётчик
    uint8_t wait;              ///< Счётчик задержки врага (при толкании мешков, изменении направления)
    uint8_t image_phase;       ///< Фаза анимации - смещение кадра в байтах (см. next_image_phase)
    int8_t image_phase_inc;    ///< Шаг фазы анимации (плюс или минус размер кадра)
    uint8_t _pad[5];           ///< Выравнивание до 16 байт
};

// Размеры структур критичны: на них завязана индексация массивов сдвигом вместо __mulhi3
static_assert(sizeof(struct bag_info) == 8);
static_assert(sizeof(struct bug_info) == 16);

/**
 * @brief Единичные шаги по направлениям (DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN).
 */
static const int8_t dir_dx[4] = { -1,  1,  0,  0 };
static const int8_t dir_dy[4] = {  0,  0, -1,  1 };

/**
 * @brief Поле состояний ячеек фона.
 */
uint8_t background[H_MAX][16];

/**
 * @brief Поле состояний монеток. Установленный бит означает наличие монетки
 */
uint16_t coins[H_MAX];

/**
 * @brief Состояние мешков с деньгами
 */
struct bag_info bags_state[MAX_BAGS];

/**
 * @brief Состояние врагов (Хоббинов/Ноббинов)
 */
struct bug_info bugs_state[MAX_BUGS];

// Переменные отвечающие за состояние Диггера
struct {
    uint8_t image_phase;       /// Фаза анимации Диггера - смещение кадра в байтах (см. next_image_phase)
    int8_t  image_phase_inc;   /// Шаг фазы анимации Диггера (плюс или минус размер кадра)
    uint16_t wait;             /// Задержка перед следующим перемещением Диггера
    uint16_t x_graph;          /// Положение Диггера по оси X в графических координатах
    uint16_t y_graph;          /// Положение Диггера по оси Y в графических координатах
    enum direction dir;        /// Направление движения Диггера
    enum direction prev_dir;   /// Предыдущее направление движения Диггера
    enum direction new_dir;    /// Желаемое новое направление движения Диггера
    enum creature_state state; /// Состояние Диггера (жив, убит, лежит дохлый)
    struct bag_info *dead_bag; /// Указатель на мешок от котрого погиб Диггер
} man;

// Переменные отвечающие за создание врагов
struct
{
    uint8_t max;           /// Максимальное количество врагов на уровне одновременно
    uint8_t total;         /// Общее количество врагов на уровне
    uint8_t delay;         /// Задержка перед рождением врага (Ноббина)
    uint8_t delay_counter; /// Счётчик задержки перед рождением врага
    uint8_t active;        /// Количество активных врагов
    uint8_t created;       /// Общее количество созданных врагов
    uint8_t boost;         /// Порог лишнего хода Ноббина (из 256)
    uint8_t swap;          /// Порог случайной смены направления врага (из 256)
} bugs;

// Переменные отвечающие за бонус-режим
struct {
    enum bonus_state state; /// Состояние режима бонус
    uint16_t time;          /// Время активности бонус-режима
    uint8_t  flash;         /// Время мерцания при включении/выключении Бонус-режима
    uint16_t count;         /// Очки за следующего пойманного в Бонус-режиме врага (200, 400, 800...)
    uint32_t life_score;    /// Количество очков для дополнительной жизни
} bonus;

// Переменные отвечающие за выстрел
struct {
    uint16_t x_graph;     /// Положение выстрела по оси X в графических координатах
    uint16_t y_graph;     /// Положение выстрела по оси Y в графических координатах
    uint8_t  image_phase; /// Фаза анимации снаряда или взрыва - смещение кадра в байтах (без умножения на размер)
    uint8_t  fire;        /// Флаг выстрела
    uint8_t  flying;      /// Флаг означающий, что снаряд летит
    uint8_t  wait;        /// Задержка готовности выстрела
    uint8_t  explode;     /// Счётчик взрывающегося снаряда
    enum direction dir;   /// Направление полёта выстрела
} mis;

// Переменные отвечающие за состояние игры
struct {
    uint16_t difficulty; /// Уровень сложности (равен номеру уровня, но не более десяти)
    uint16_t level;      /// Текущий номер уровня (начиная с единицы)
    uint16_t level_no;   /// Номер экрана (поля) текущего уровня (индекс в массиве level[])
    int16_t  lives;      /// Текущее количество жизней
    uint32_t score;      /// Количество очков
} game;

uint8_t broke_max; // Время через которое исчезнет разбившийся мешок

// Счётчик "отрисовок" за кадр, ведётся как inc_plot() в оригинале: в перегруженном
// кадре враги притормаживаются (delay_bugs)
uint16_t plot_count;

// Переменные отвечающие за вывод звуков.
uint16_t snd_effects = 1;    /// Флаг, показывающий, что звуковые эффекты включены
uint16_t music_on = 1;       /// Флаг, показывающий, что фоновая музыка включена

struct {
    uint8_t  loose;               /// Флаг, означающий, что звук качающегося мешка включен
    uint16_t loose_snd_phase;     /// Фаза звука качающегося мешка

    uint8_t  fall;                /// Флаг, означающий, что звук летящего мешка включен
    uint8_t  fall_snd_phase;      /// Фаза звука падающего мешка
    uint16_t fall_period;         /// Период звука летящего мешка
    uint8_t  break_bag;           /// Флаг, означающий, что звук разбивающегося мешка включен
    uint8_t  money;               /// Счётчик 0..30 - звук съедания золота
    uint16_t money_period_1;      /// Период звука съедания золота (нечётные ноты)
    uint16_t money_period_2;      /// Период звука съедания золота (чётные ноты)

    uint8_t  coin;                /// Счётчик 0..7 - звук съедания монетки/драгоценного камня
    int8_t   coin_note;           /// Номер ноты при съедании монетки (драгоценного камня)
    uint8_t  coin_time;           /// Таймер между последовательными съедениями драгоценных камней (монеток)

    uint8_t  fire;                /// Флаг, означающий, что звук выстрела включен
    uint16_t fire_period;         /// Период звука выстрела
    uint8_t  explode;             /// Флаг, означающий, что звук взрыва включен

    uint8_t  chase;               /// Счётчик 0..19 - звук включения/выключения бонус-режима
    uint8_t  chase_flip;          /// Флаг переключающий тональность звука бонус-режима

    uint8_t  bug;                 /// Флаг, означающий, что звук съедания врага в бонус-режиме включен
    uint8_t  bug_c1;              /// Счётчик 20..0 элементов звука съедения врага
    uint8_t  bug_c2;              /// Счётчик 4..0 фазы звука съедения врага
    uint16_t bug_period;          /// Текущий затухающий период звука съедения врага
    uint16_t bug_period_held;     /// Удерживаемое значение периода (звучит 2 ноты подряд)

    uint8_t  life;                /// Счётчик 0..24 - звук получения дополнительной жизни
    uint8_t  done;                /// Флаг, означающий, что звук завершения уровня включен
} snd;

#ifdef MINIMAP
/**
 * @brief Отладочная процедура отображения мини-карты состояния фона
 */
static void draw_bg_minimap()
{
    sp_put(48, SCREEN_Y_OFFSET + MOVE_Y_STEP + 2, sizeof(background[0]), sizeof(background) / sizeof(background[0]), (uint8_t*)background, 0);
}

/**
 * @brief Отладочная процедура отображения мини-карты состояния монеток (драгоценных камней)
 */
static void draw_coin_minimap()
{
    sp_put(45, SCREEN_Y_OFFSET + MOVE_Y_STEP + 2, sizeof(coins[0]), sizeof(coins) / sizeof(coins[0]), (uint8_t*)coins, 0);
}
#endif

static int remove_coin(uint8_t x_log, uint8_t y_log);

/**
 * @brief Вывод десятичного числа в диапазоне 0..MAX_SCORE в SCORE_DIGITS знаков с ведущими нулями
 *
 * @param number - число для вывода; ограничить его сверху значением MAX_SCORE обязан вызывающий:
 *                 на большем значении старший разряд получится больше девяти и индексация
 *                 digit_indices[digit] уйдёт за границу таблицы
 * @param x_graph - координата X по которой будет осуществлён вывод числа
 * @param y_graph - координата Y по которой будет осуществлён вывод числа
 */
static void print_dec(uint32_t number, uint16_t x_graph, uint16_t y_graph)
{
    constexpr uint16_t row_w = sizeof(digit_rows[0]); // 3 байта на строку
    constexpr uint16_t row_n = sizeof(digit_indices[0]) / sizeof(digit_indices[0][0]); // 12 строк на цифру

    // Разряды выделяются вычитанием степеней десяти (не более 9 вычитаний на разряд).
    // Деление здесь тянуло бы из libgcc 32-битный __udivsi3, а вычитание по единице
    // (как в uint_to_str) на шестизначном счёте дало бы до 111 тысяч итераций.
    static const uint32_t pow_10[SCORE_DIGITS] = { 100000UL, 10000UL, 1000UL, 100UL, 10UL, 1UL };

    // Таблица обходится указателем, а не индексом: на индексе gcc грузит слова
    // 32-битного элемента как "mov tbl(r0),r0 / mov tbl+2(r0),r1" и затирает
    // индекс первым же mov, отчего в старшие разряды попадают чужие константы.
    const uint32_t *pw = pow_10;

    uint8_t digit_buf[row_n * row_w];
    for (uint8_t i = 0; i < SCORE_DIGITS; ++i)
    {
        const uint32_t p = *pw++;
        // Строка таблицы цифры выбирается сдвигом указателя, а не индексом: digit * 12 стоило бы __mulhi3
        const uint8_t *idx_row = digit_indices[0];
        while (number >= p)
        {
            number -= p;
            idx_row += row_n;
        }

        uint8_t *dst = digit_buf;
        for (uint8_t r = 0; r < row_n; ++r)
        {
            const uint8_t *src = &digit_rows[0][0] + idx_row[r]; // В таблице уже смещения в байтах
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src;
        }
        sp_put(x_graph, y_graph, row_w, row_n, digit_buf, nullptr);
        x_graph += row_w;
    }
}

/**
 * @brief Вывод количества жизней в виде спрайтов Диггера рядом с количеством очков
 */
static void print_lives()
{
    uint16_t man_x_offset = sizeof(digit_rows[0]) * SCORE_DIGITS + 1; // Смещение шириной в счётчик очков плюс один байт (4 пикселя)
    constexpr uint16_t man_y_offset = SCREEN_Y_OFFSET + 2; // Смещение спрайта Диггера по оси Y
    constexpr uint16_t one_pos_width = sizeof(image_digger_right[1][0]) + 1; // Ширина спрайта Диггера плюс один байт
    constexpr uint16_t height = sizeof(image_digger_right[1]) / sizeof(image_digger_right[1][0]); // Высота спрайта Диггера
    int16_t width = MAX_LIVES * one_pos_width; //  Общий размер места занимаемый спрайтами Диггера
    uint8_t *sprite = (uint8_t *)image_digger_right[2];

    for (uint16_t l = 1; width > 0; man_x_offset += one_pos_width, width -= one_pos_width)
    {
        asm ("" : "+r"(man_x_offset), "+r"(width)); // Иначе gcc выводит их из l вызовом __mulhi3

        if (++l > game.lives)
        {
            sp_clear_brick(man_x_offset, man_y_offset, width, height);
            break;
        }

        sp_4_15_put(man_x_offset, man_y_offset, sprite);
    }
}

/**
 * @brief Добавление заданного количества очков и вывод очков в левом верхнем углу экрана
 */
static void add_score(uint16_t score_add)
{
    plot_count += 3;
    game.score += score_add;
    // Счёт шестизначный: при переполнении обнуляется (как в оригинале). bonus.life_score при этом
    // намеренно не сбрасывается - в оригинале bonus растёт сквозь обнуление счёта,
    // поэтому после переполнения бонусные жизни до конца партии не выдаются.
    if (game.score > MAX_SCORE) game.score = 0;
    print_dec(game.score, 0, SCREEN_Y_OFFSET + MOVE_Y_STEP);

     // Если количество очков достигло бонусного для получения жизни
    if (game.score >= bonus.life_score)
    {
        bonus.life_score += BONUS_LIFE_SCORE; // Количество очков до следующего бонуса в виде жизни

        // Выдать жизнь только если максимум ещё не достигнут
        if (game.lives < MAX_LIVES)
        {
            game.lives++; // Увеличичить количество жизней на единицу
            print_lives(); // Вывести количество жизней
            snd.life = 24; // Издать звук получения жизни
        }
    }
}

/**
 * @brief Добавление очков за убитого врага
 */
__attribute__((noinline)) static void add_score_250() // Вызов без аргументов короче встроенной передачи 250
{
    add_score(250); // 250 очков за убитого врага
}

/**
 * @brief Проверка соприкосновения двух 4x15-спрайтов по их левым-верхним углам.
 */
static int check_collision_4_15(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    return ((uint16_t)((int)x2 - (int)x1 + 3) < 7u)
        && ((uint16_t)((int)y2 - (int)y1 + 14) < 29u);
}

/**
 * @brief Проверка соприкосновения снаряда (2x7) с 4x15-спрайтом по их левым-верхним углам.
 *
 * Габариты снаряда меньше, поэтому проверять его через check_collision_4_15 нельзя:
 * зона попадания оказывалась бы раздута вправо на 2 байта и вниз на 8 строк, и
 * летящий в эту сторону снаряд взрывался бы, не долетев до врага полклетки.
 */
static int check_collision_missile(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    return ((uint16_t)((int)x2 - (int)x1 + 3) < 5u)   // x2 - x1 в пределах -3..1
        && ((uint16_t)((int)y2 - (int)y1 + 14) < 21u); // y2 - y1 в пределах -14..6
}

/**
 * @brief Преобразование графической координаты X в логическую (номер клетки).
 */
static inline uint8_t graph_to_x_log(uint16_t x_graph)
{
    // Знаковый сдвиг - это asr, беззнаковый - пара clc/ror на каждый разряд. Отрицательной
    // разность бывает только у снаряда за левым краем: клетка и так выходит за W_MAX
    static_assert(POS_X_STEP == 4);
    return (int16_t)(x_graph - FIELD_X_OFFSET) >> 2;
}

// Деление на POS_Y_STEP разворачивается в цикл сдвигов, который при девяти местах вызова
// стоит дороже, чем jsr (см. graph_to_x_log - там сдвигов два, инлайн выгоднее).
__attribute__((noinline)) static uint8_t graph_to_y_log(uint16_t y_graph)
{
    return (y_graph - FIELD_Y_OFFSET) / POS_Y_STEP;
}

/**
 * @brief Положение внутри клетки (номер подпозиции): 0 - объект выровнен по клетке.
 */
static inline uint16_t graph_to_x_rem(uint16_t x_graph)
{
    return (x_graph - FIELD_X_OFFSET) % POS_X_STEP;
}

static inline uint16_t graph_to_y_rem(uint16_t y_graph)
{
    // Знаковый сдвиг (asr) короче беззнакового, а после маски значение неотрицательно
    static_assert(POS_Y_STEP == 16 && MOVE_Y_STEP == 4);
    return (int16_t)((y_graph - FIELD_Y_OFFSET) & (POS_Y_STEP - 1)) >> 2;
}

/**
 * @brief Умножение на 3 и на 5 сдвигом и сложением
 *
 * Без барьера gcc сворачивает сдвиг со сложением обратно в вызов __mulhi3. Пока ни
 * одного вызова нет, __mulhi3 не попадает в сборку вовсе, а каждый вызов с передачей
 * аргументов через стек длиннее сдвигов.
 */
static inline uint16_t mul3(uint16_t x)
{
    uint16_t x2 = x << 1;
    asm ("" : "+r"(x2));
    return x + x2;
}

static inline uint16_t mul5(uint16_t x)
{
    uint16_t x4 = x << 2;
    asm ("" : "+r"(x4));
    return x + x4;
}

/**
 * @brief Получение ячейки уровня в заданной строке.
 *
 * @param row - строка уровня (level[номер экрана][y_log])
 * @param x_log - логическая координата по оси X
 */
static inline enum level_symbols getLevelSymbol(const uint16_t *row, uint8_t x_log)
{
    static const uint8_t word_no_tbl[W_MAX] = { 0,0,0,0,0, 1,1,1,1,1, 2,2,2,2,2 };
    static const uint8_t shift_tbl[W_MAX]   = { 0,3,6,9,12, 0,3,6,9,12, 0,3,6,9,12 };

    return (row[word_no_tbl[x_log]] >> shift_tbl[x_log]) & 7;
}

static void bonus_indicator(uint16_t color);

/**
 * @brief Вернуть экран в нормальное положение
 *
 * Анимация разбивающегося мешка "проваливает" экран на один кадр. Если в этот момент
 * Диггер погиб, мешок деактивируется вместе с остальным состоянием уровня и вернуть
 * регистр на место будет уже некому.
 */
static void reset_v_scroll()
{
    *((volatile uint16_t *)REG_V_SCROLL) = 0330 | (1 << V_SCROLL_EXT_MEMORY);
}

// Размер кадра спрайта 4x15 в байтах
constexpr uint8_t FRAME_4_15 = sizeof(image_nobbin[0]);

/**
 * @brief Переключить фазу анимации спрайта по циклу 0-1-2-1-...
 *
 * Одинаково анимируются и Диггер, и враги, поэтому шаг фазы с разворотом
 * на границах вынесен из draw_man и move_bug сюда. Фаза хранится как смещение
 * кадра в байтах (0, FRAME_4_15, 2 * FRAME_4_15): индекс пришлось бы умножать
 * на размер кадра вызовом __mulhi3.
 *
 * @param phase - текущая фаза
 * @param inc - шаг фазы (+FRAME_4_15 или -FRAME_4_15)
 */
static void next_image_phase(uint8_t *phase, int8_t *inc)
{
    *phase += *inc;
    if (!*phase || *phase >= 2 * FRAME_4_15) *inc = -*inc;
}

/**
 * @brief Нарисовать мешок с золотом по заданным координатам
 */
__attribute__((noinline)) static void draw_bag(uint16_t x_graph, uint16_t y_graph) // Два аргумента вместо четырёх
{
    sp_4_15_mask(x_graph, y_graph, image_bag[0], outline_bag[0]);
}

/**
 * @brief Стереть мешок с золотом по заданным координатам
 */
static void erase_bag(uint16_t x_graph, uint16_t y_graph)
{
    sp_4_15_mask(x_graph, y_graph, nullptr, outline_bag[0]);
}

/**
 * @brief Инициализация переменных состояния перед старом уровня
 */
static void init_level_state()
{
    reset_v_scroll(); // Вернуть экран в нормальное положение

    // Отключить бонус-режим
    bonus.state = BONUS_OFF;

    // Стереть вишенку
    erase_4_15(CORNER_X, CORNER_Y);

    // Отключение индикации бонус-режима
    bonus_indicator(0);

    // Деактивировать всех врагов
    for (uint8_t i = 0; i < MAX_BUGS; ++i)
    {
        struct bug_info *bug = &bugs_state[i]; // Структура с информацией о враге
        if (bug->state == CREATURE_INACTIVE) continue; // Пропустить неактивных врагов

        erase_4_15(bug->x_graph, bug->y_graph); // Стереть врага
        bug->state = CREATURE_INACTIVE; // Деактивировать врага
    }

    // Деактивировать нестационарные мешки
    for (uint16_t i = 0; i < MAX_BAGS; ++i)
    {
        struct bag_info *bag = &bags_state[i];  // Структура с информацией о мешке
        if (bag->state == BAG_INACTIVE) continue; // Пропустить неактивные мешки
        if ((bag->state == BAG_STATIONARY) && (bag->dir == DIR_STOP)) continue; // Пропустить стационарные мешки

        erase_bag(bag->x_graph, bag->y_graph); // Стереть мешок
        bag->state = BAG_INACTIVE;
    }

    // print_dec(game.difficulty, 0, MAX_Y_POS + 2 * POS_Y_STEP);

    if (game.difficulty > 7) bugs.max = 5;      // На уровне сложности 8 и выше максимально 5 врагов одновременно
    else if (game.difficulty > 1) bugs.max = 4; // На уровне сложности со 2 до 7 (включительно) до 4 врагов одновременно
    else bugs.max = 3;                      // На первом уровне максимально три врага одновременно

    // Переменные относщиеся к созданию и управлению врагами
    bugs.total = game.difficulty + 5;         // Общее количество врагов на уровне - пять плюс уровень сложности
    bugs.delay = 45 - (game.difficulty << 1); // Задержка появления врагов (с ростом сложности убывает)
    bugs.delay_counter = 10;             // Первый враг - через 10 тактов после старта (как в оригинале)
    bugs.active = 0;                     // Количество активных врагов
    bugs.created = 0;                    // Общее количество созданных врагов

    // Порог ⌈256/(16-d)⌉ - ровно вероятность rnd(15-d)==0 оригинала (rnd_table - перестановка 0..255)
    static const uint8_t bug_boost[10] = { 18, 19, 20, 22, 24, 26, 29, 32, 37, 43 };
    bugs.boost = bug_boost[game.difficulty - 1];

    // Порог для rnd(5+d)==1 оригинала: ⌈512/(6+d)⌉-⌈256/(6+d)⌉; с шестого уровня случайности нет
    static const uint8_t bug_swap[10] = { 37, 32, 28, 26, 23, 0, 0, 0, 0, 0 };
    bugs.swap = bug_swap[game.difficulty - 1];

    broke_max = 150 - mul5(game.difficulty << 1); // Время через которое исчезнет разбившийся мешок (с ростом сложности убывает)

    // Инициализация переменных Диггера
    man.dir = DIR_RIGHT;
    man.prev_dir = DIR_RIGHT;
    man.x_graph = FIELD_X_OFFSET + MAN_START_X * POS_X_STEP; // Исходная координата Диггера на экране по оси X
    man.y_graph = FIELD_Y_OFFSET + MAN_START_Y * POS_Y_STEP; // Исходная координата Диггера на экране по оси Y
    man.image_phase = 0;        // Фаза анимации Диггера
    man.image_phase_inc = FRAME_4_15; // Направление изменения фазы анимации Диггера
    man.wait = 0;               // Задержка перед следующим перемещением Диггера
    man.state = CREATURE_ALIVE; // Исходное состояние - Диггер жив

    // Инициализация переменных снаряда
    mis.fire = 0;
    mis.flying = 0;
    mis.wait = 0;
    mis.explode = 0;

    // Инициализация переменных используемых для звуковых эффектов.
    clr_words(&snd, sizeof(snd) / 2);
    snd.coin_note = -1;
    snd.coin_time = 0;

    bg_music_track(BG_MUSIC_POPCORN); // Начать фоновую музыку (Popcorn) с начала
}

/**
 * @brief Прогрызть фон в соответствии с направлением движения и текущим положением
 *
 * @param dir - направление движения
 * @param x_graph - графическая координата по оси X
 * @param y_graph - графическая координата по оси Y
 */
static void gnaw(enum direction dir, uint16_t x_graph, uint16_t y_graph)
{
    static const int8_t   gnaw_x[4] = { -2, 4, -1, -1 };
    static const int8_t   gnaw_y[4] = { -1, -1, -7, 15 };
    static const uint8_t  gnaw_x_size[4] = {
        sizeof(outline_blank_left[0]), sizeof(outline_blank_right[0]), sizeof(outline_blank_up[0]), sizeof(outline_blank_down[0])
    };
    static const uint8_t  gnaw_y_size[4] = {
        sizeof(outline_blank_left)  / sizeof(outline_blank_left[0]),  sizeof(outline_blank_right) / sizeof(outline_blank_right[0]),
        sizeof(outline_blank_up)    / sizeof(outline_blank_up[0]),    sizeof(outline_blank_down)  / sizeof(outline_blank_down[0])
    };
    static uint8_t *const gnaw_sprite[4] = {
        (uint8_t*)outline_blank_left, (uint8_t*)outline_blank_right, (uint8_t*)outline_blank_up, (uint8_t*)outline_blank_down
    };

    sp_put(x_graph + gnaw_x[dir], y_graph + gnaw_y[dir], gnaw_x_size[dir], gnaw_y_size[dir], nullptr, gnaw_sprite[dir]);
}

/**
 * @brief Установка номера экрана и уровня сложности по текущему номеру уровня.
 *
 *        Как в оригинале (get_screen(), get_dificulty()): экраны 1-8 соответствуют
 *        уровням 1-8, а начиная с девятого уровня циклически повторяются только
 *        экраны 5-8 в порядке 6, 7, 8, 5 (уровень 9 - экран 6, уровень 12 - экран 5).
 *        Уровень сложности равен номеру уровня и ограничен сверху десятью.
 */
static void set_level_params()
{
    // Номер экрана (1-8) для текущего номера уровня
    uint16_t screen = game.level;
    if (screen > LEVELS_NUM) screen = (game.level & 3) + 5; // (level % 4) + 5

    game.level_no = screen - 1; // Индекс экрана в массиве уровней

    game.difficulty = game.level;
    if (game.difficulty > 10) game.difficulty = 10;
}

/**
 * @brief Инициализация уровня (отрисовка фона, расстановка монеток и мешков, отрисовка прогрызенных проходов)
 */
static void init_level()
{
    clr_words(bags_state, sizeof(bags_state) / 2); // Деактивировать все мешки
    clr_words(bugs_state, sizeof(bugs_state) / 2); // Деактивировать всех врагов

    constexpr uint16_t bg_block_width = sizeof(image_background[0][0]); // Ширина блока фона
    constexpr uint16_t bg_block_height = sizeof(image_background[0]) / sizeof(image_background[0][0]); // Высота блока фона

    constexpr uint16_t x_size = 13; // Ширина поля фона в блоках
    constexpr uint16_t y_size = POS_Y_STEP * H_MAX / bg_block_height + MOVE_Y_STEP + 2; // Высота поля фона в блоках

    static_assert(sizeof(image_background[0]) == 20);
    const uint8_t *back_image = (uint8_t *)image_background + mul5(game.level_no << 2); // Образец фона текущего уровня: image_background[game.level_no]

    // Отрисовка фона
    for (uint16_t y_graph = 0; y_graph < y_size * bg_block_height; y_graph += bg_block_height)
    {
        for (uint16_t x_graph = 0; x_graph < x_size * bg_block_width; x_graph += bg_block_width)
        {
            sp_put(x_graph - 1, y_graph + FIELD_Y_OFFSET - MOVE_Y_STEP * 3, bg_block_width, bg_block_height, back_image, nullptr);
        }
    }

    // Инициализация поля фона, поля моненток, состояний мешков
    uint16_t bag_num = 0;
    uint16_t x_graph = FIELD_X_OFFSET;
    uint16_t y_graph = FIELD_Y_OFFSET;
    static_assert(sizeof(level[0]) / sizeof(level[0][0][0]) == 30);
    const uint16_t *level_row = &level[0][0][0] + mul5(mul3(game.level_no << 1)); // level[game.level_no][0]
    for (uint16_t y_log = 0; y_log < H_MAX; ++y_log, level_row += W_MAX / CELLS_PER_WORD)
    {
        coins[y_log] = 0; // Сброситть все биты монеток для данной строки

        for (uint16_t x_log = 0; x_log < W_MAX; ++x_log)
        {
            uint8_t *bg = &background[y_log][x_log]; // Структура с информацией о клетке фона
            *bg = 0; // Сбросить все биты состояния фона (вся клетка фона цела)

            enum level_symbols ls = getLevelSymbol(level_row, x_log);

            if (ls == LEV_C)
            {
                coins[y_log] |= 1 << x_log; // Установить бит соответствующий монетке на карте уровня
                // Нарисовать монетку (драгоценный камень)
                sp_put(x_graph, y_graph + COIN_Y_OFFSET, sizeof(image_coin[0]), sizeof(image_coin) / sizeof(image_coin[0]), (uint8_t *)image_coin, (uint8_t *)outline_coin);
            }
            else if (ls == LEV_B)
            {
                struct bag_info *bag = &bags_state[bag_num++]; // Структура с информацией об очередном мешке

                bag->state = BAG_STATIONARY; // Мешок стоит на месте
                bag->count = 0;              // Счётчик сброшен
                bag->x_graph = x_graph;      // Координата мешка по оси X
                bag->y_graph = y_graph;      // Координата мешка по оси Y
                bag->dir = DIR_STOP;         // Мешок стоит на месте

                // Нарисовать мешок с золотом
                draw_bag(bag->x_graph, bag->y_graph);
            }

            if (ls == LEV_H || ls == LEV_S)
            {
                *bg |= 0x0F;  // устанавливаем все биты состояния фона для горизонтальных проходов
                for (uint16_t i = 4; i > 0; --i)
                {
                    gnaw(DIR_RIGHT, x_graph - i, y_graph);
                }
                gnaw(DIR_LEFT, x_graph + 1, y_graph);
            }

            if (ls == LEV_V || ls == LEV_S)
            {
                *bg |= 0xF0;  // устанавливаем все биты состояния фона для вертикальных проходов
                // Пять прогрызов сверху вниз с шагом 3. Число итераций такого цикла gcc вычисляет
                // делением на 3 через __mulhi3 - барьер оставляет ему простой счётчик
                uint16_t gnaw_y = y_graph - 15;
                for (uint8_t n = 5; n; --n, gnaw_y += 3)
                {
                    asm ("" : "+r"(gnaw_y));
                    gnaw(DIR_DOWN, x_graph, gnaw_y);
                }
                gnaw(DIR_UP, x_graph, y_graph + 3);
            }

            x_graph += POS_X_STEP;
        }

        x_graph = FIELD_X_OFFSET;
        y_graph += POS_Y_STEP;
    }

    init_level_state();  // Инициализировать состояние уровня (в т.ч. сброс фоновой музыки на начало)
}

/**
 * @brief Определяет по состоянию байта клетки, что клетка проедена насквозь
 *
 * Прохождение клетки требует, чтобы она была очищена целиком по одной из осей:
 * 0x0F - горизонтальный проход, 0xF0 - вертикальный. Оба спрайта (и Диггер, и враг)
 * занимают клетку целиком, поэтому вход в клетку, прогрызенную лишь с краю, оставляет
 * прямоугольник затёртой земли вместо аккуратного прогрыза.
 *
 * @param byte - байт с состоянием клетки
 *
 * @return - ненулевое значение, если клетка проедена
 */
static uint16_t full_bite(uint8_t byte)
{
    return (((byte & 0x0F) == 0x0F) || ((byte & 0xF0) == 0xF0));
}

/**
 * @brief Определяет возможность движения в заданном направлении
 *
 * @param dir - направлкние движения
 * @param x_graph - графическая координата по оси X
 * @param y_graph - графическая координата по оси Y
 *
 * @return - 1 - движение в заданном направлении возможно, 0 - движение в заданном направлении невозможно
 */
static uint8_t check_path(enum direction dir, uint16_t x_graph, uint16_t y_graph)
{
    uint8_t x_log = graph_to_x_log(x_graph);
    uint8_t y_log = graph_to_y_log(y_graph);
    const uint8_t current_cell = background[y_log][x_log]; // Состояние текущей клетки

    // Смещение до соседней клетки берётся из общих dir_dx/dir_dy, здесь только маски:
    // mask - ближняя к текущей клетке сторона соседней, cur_mask - дальняя сторона текущей
    static const struct
    {
        uint8_t mask;
        uint8_t cur_mask;
    } dir_matrix[4] = {
        { 0x08, 0x01 }, // Влево
        { 0x01, 0x08 }, // Вправо
        { 0x80, 0x10 }, // Вверх
        { 0x10, 0x80 }  // Вниз
    } ;

    x_log += dir_dx[dir];
    y_log += dir_dy[dir];

    if ((x_log >= W_MAX) || (y_log >= H_MAX)) return 0;

    const uint8_t neighbor_cell = background[y_log][x_log]; // Состояние соседней клетки
    if (!full_bite(neighbor_cell)) return 0;
    if (neighbor_cell & dir_matrix[dir].mask) return 1;
    return current_cell & dir_matrix[dir].cur_mask;
}

/**
 * @brief Очистить биты состояния фона по заданным координатам в указанном направлении
 *
 * @param x_graph - графическая координата по оси X
 * @param y_graph - графическая координата по оси Y
 * @param dir - направлкние движения
 */
static void set_background_bits(uint16_t x_graph, uint16_t y_graph, enum direction dir)
{
    uint16_t x_log = graph_to_x_log(x_graph);
    uint16_t y_log = graph_to_y_log(y_graph);
    int16_t x_rem = graph_to_x_rem(x_graph);
    int16_t y_rem = graph_to_y_rem(y_graph);

    switch (dir)
    {
        case DIR_LEFT:
        {
            if (--x_rem < 0)
            {
                x_rem += 4;
                x_log--;
            }

            break;
        }

        case DIR_RIGHT:
        {
            // if (++x_rem >= 4)
            // {
            //     x_rem -= 4;
            //     x_log++;
            // }

            x_log++;

            break;
        }

        case DIR_UP:
        {
            if (--y_rem < 0)
            {
                y_rem += 4;
                y_log--;
            }

            break;
        }

        case DIR_DOWN:
        {
            // if (++y_rem >= 4)
            // {
            //     y_rem -= 4;
            //     y_log++;
            // }

            y_log++;

            break;
        }
    }

     // Проверка на выход за пределы игрового поля
    if (x_log >= W_MAX || y_log >= H_MAX) return;

    uint8_t *cell = &background[y_log][x_log]; // Указатель на текущую ячейку состояния фона

    // Маски таблицей: сдвиг на переменное число разрядов на PDP-11 - это цикл
    static const uint8_t bit_mask[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

    switch (dir)
    {
        case DIR_LEFT:
        case DIR_RIGHT:
        {
            *cell |= bit_mask[x_rem]; // Установить соответсвующий бит матрицы фона
            break;
        }

        case DIR_UP:
        case DIR_DOWN:
        {
            *cell |= bit_mask[y_rem + 4]; // Установить соответсвующий бит матрицы фона
            break;
        }
    }
}

/**
 * @brief Проверка на выход за пределы игрового поля
 */
static int check_out_of_range(enum direction dir, uint16_t x_graph, uint16_t y_graph)
{
    return (
        (dir == DIR_RIGHT && x_graph >= MAX_X_POS) ||
        (dir == DIR_LEFT  && x_graph <= MIN_X_POS) ||
        (dir == DIR_DOWN  && y_graph >= MAX_Y_POS) ||
        (dir == DIR_UP    && y_graph <= MIN_Y_POS)
    );
}

/**
 * @brief Проверка на то, что перемещение по оси X происходит на заданный объект
 *
 * @param dir - направление перемещения
 * @param x_graph - координата X перемещаемого объекта
 * @param object_x_graph - координата X объекта на который возможно перемещение
 */
static int move_to_object(enum direction dir, uint16_t x_graph, uint16_t object_x_graph)
{
    // Если направление перемещения вправо и объект находится правее
    // или направление перемещения влево и объект находтся левее
    return (((dir == DIR_RIGHT) && (object_x_graph > x_graph)) ||
            ((dir == DIR_LEFT)  && (x_graph > object_x_graph)));
}

/**
 * @brief Переместить определённый мешок.
 * Если будут затронуты другие мешки, перемещение будет отменено и возвращено значение 0.
 *
 * @param bag - указатель на структуру с информацией о мешке
 * @param dir - направление перемещения мешка
 * @return 0 - мешок был перемещён, 1 - мешок не был перемещён
 */
static uint8_t move_bag(struct bag_info *bag, enum direction dir)
{
    uint8_t rv = 0;

    uint16_t bag_x_graph = bag->x_graph;
    uint16_t bag_y_graph = bag->y_graph;

    // Проверить пытается ли переместиться мешок за пределы экрана
    if (check_out_of_range(dir, bag_x_graph, bag_y_graph))
    {
        return 1; // Если мешок пытается переместиться за пределы экрана, отменить перемещение
    }
    else
    {
        // Если раскачивающийся мешок двигают встороны, то он перестаёт раскачиваться
        if (bag->state == BAG_LOOSE)
        {
            bag->state = BAG_STATIONARY;
            bag->count = 0;
        }

        switch (dir)
        {
            case DIR_RIGHT:
            {
                bag_x_graph += MOVE_X_STEP; // Перемещение мешка на шаг вправо
                break;
            }

            case DIR_LEFT:
            {
                bag_x_graph -= MOVE_X_STEP;  // Перемещение мешка на шаг влево
                break;
            }
        }

        // Проверить перемещается ли мешок на Диггера
        if (check_collision_4_15(bag_x_graph, bag_y_graph, man.x_graph, man.y_graph))
        {
            if (move_to_object(dir, bag_x_graph, man.x_graph))
            {
                rv = 1; // Если да, отменить перемещение
            }
        }

        // Проверить перемещается ли мешок на врага
        for (uint8_t i = 0; i < bugs.max; ++i)
        {
            struct bug_info *bug = &bugs_state[i]; // Структура с информацией о враге
            if (bug->state != CREATURE_ALIVE) continue; //  Пропустить неживых врагов

            // Проверить, что мешок перемещается на врага
            if (check_collision_4_15(bag_x_graph, bag_y_graph, bug->x_graph, bug->y_graph))
            {
                if (move_to_object(dir, bag_x_graph, bug->x_graph))
                {
                    rv = 1; // Если да, отменить перемещение
                    break;
                }
            }
        }

        // Проверка соприкосновение с другими мешками
        for (uint8_t i = 0; i < MAX_BAGS; ++i)
        {
            struct bag_info *another_bag = &bags_state[i]; // Структура с информацией о мешке

            if (another_bag == bag) continue; // Пропустить мешок, процедуру обработки которого вызвали

             // Пропустить не стационарные и не качающиеся мешки
            if ((another_bag->state != BAG_STATIONARY) && (another_bag->state != BAG_LOOSE)) continue;

            // Проверить, что мешок соприкоснулся с другим мешком
            if (check_collision_4_15(bag_x_graph, bag_y_graph, another_bag->x_graph, another_bag->y_graph))
            {
                // Если направление перемещения вправо и другой мешок находится правее обрабатываемого мешка
                // или направление перемещения влево и другой мешок находтся левее обрабатываемого мешка
                if (move_to_object(dir, bag_x_graph, another_bag->x_graph))
                {
                    // Попробовать взывать перемещение мешка с которым обнаружена коллизия
                    if (move_bag(another_bag, dir))
                    {
                        // Если другой мешок не смог переместиться
                        rv = 1; // Отменить перемещение и этого мешка
                        break;
                    }
                }
            }
        }
    }

    if (!rv)
    {
        plot_count++;

        // Стирание мешка по старым координатам
        erase_bag(bag->x_graph, bag->y_graph);

        // Отрисовка спрайта передвигаемого мешка
        draw_bag(bag_x_graph, bag_y_graph);

        set_background_bits(bag_x_graph, bag_y_graph, dir); // Сбросить биты матрицы фона
        // Удалить монеты уничтоженные мешком
        remove_coin(graph_to_x_log(bag_x_graph), graph_to_y_log(bag_y_graph));

        // Установить новые координаты мешка
        bag->dir = dir;
        bag->x_graph = bag_x_graph;
        bag->y_graph = bag_y_graph;
    }

    return rv;
}

/**
 * @brief Стереть след за объектом размером 15x4
 *
 * @param dir     - направление движения объекта
 * @param x_graph - графическая координата по оси X
 * @param y_graph - графическая координата по оси Y
 */
static void erase_trail(enum direction dir, uint16_t x_graph, uint16_t y_graph)
{
    static const int8_t trail_dx[4]   = {  4, -MOVE_X_STEP,  0,            0 };
    static const int8_t trail_dy[4]   = {  0,            0, 15, -MOVE_Y_STEP };
    static const uint8_t trail_w[4]   = { MOVE_X_STEP, MOVE_X_STEP, 4, 4 };
    static const uint8_t trail_h[4]   = { 15, 15, MOVE_Y_STEP, MOVE_Y_STEP };

    sp_clear_brick(x_graph + trail_dx[dir], y_graph + trail_dy[dir], trail_w[dir], trail_h[dir]);
}

/**
 * @brief Обработка перемещения Ноббина/Хоббина
 *
 * @param bug - указатель на структуру с информацией о враге
 */
static void move_bug(struct bug_info *bug)
{
    enum direction dir_1, dir_2, dir_3, dir_4;

    const uint16_t bug_x_graph = bug->x_graph;
    const uint16_t bug_y_graph = bug->y_graph;
    const uint8_t bug_x_rem = graph_to_x_rem(bug_x_graph);
    const uint8_t bug_y_rem = graph_to_y_rem(bug_y_graph);

    // Проверка возможности изменения направления движения при нахождении на ровной границе клетки
    if (!bug_x_rem && !bug_y_rem)
    {
        // Если Хоббин застрял на время более заданного, то превратить его в Ноббина
        if ((bug->type == BUG_HOBBIN) && (bug->count > (30 + game.difficulty * 2)))
        {
            bug->count = 0;         // Очистить время застревания
            bug->type = BUG_NOBBIN; // Превратить врага в Ноббина
        }

        // Поиск порядка наилучших направлений движения
        dir_1 = (man.x_graph < bug_x_graph) ? DIR_LEFT : DIR_RIGHT;
        dir_2 = (man.y_graph < bug_y_graph) ? DIR_UP   : DIR_DOWN;

        // Если расстояние по вертикали превышает горизонтальное — отдать приоритет оси Y.
        // Расстояния сравниваются в пикселях оригинала (клетка 20x18): здесь клетка - 4 байта
        // по X и 16 строк по Y, поэтому 18*dy/16 > 20*dx/4, т.е. 9*dy > 40*dx
        const uint16_t dist_x = abs16(man.x_graph - bug_x_graph);
        const uint16_t dist_y = abs16(man.y_graph - bug_y_graph);
        uint16_t dist_y9 = dist_y << 3;
        uint16_t dist_x40 = dist_x << 2;
        asm ("" : "+r"(dist_y9), "+r"(dist_x40)); // Иначе gcc свернёт сдвиги в вызовы __mulhi3
        dist_y9 += dist_y;
        dist_x40 = (dist_x40 + dist_x) << 3;
        if (dist_y9 > dist_x40)
        {
            // Обмен dir_1 <-> dir_2 (XOR-swap): первичной становится ось Y
            dir_1 ^= dir_2;
            dir_2 ^= dir_1;
            dir_1 ^= dir_2;
        }

        dir_3 = dir_2 ^ 1;
        dir_4 = dir_1 ^ 1;

        // Если включён режим Бонус, то поменять порядок направлений чтобы враги разбегались
        if (bonus.state == BONUS_ON)
        {
            // Наиболее приоритетное направление поменять с наименее приоритетным
            dir_1 ^= dir_4;
            dir_4 ^= dir_1;
            dir_1 ^= dir_4;

            // Более приоритетное поменять с менее приоритетным
            dir_2 ^= dir_3;
            dir_3 ^= dir_2;
            dir_2 ^= dir_3;
        }

        // Сделать движение назад последним выбором при определении направления
        enum direction dir = (bug->dir) ^ 1; // Инвертировать направление движения врага

        // Все четыре направления различны, поэтому движение назад совпадает ровно с одним
        // из них: находим его и переносим в конец списка, сдвигая более слабые на его место
        if (dir == dir_1) // Если движение назад наболее приоритетно
        {
            dir_1 = dir_2;
            dir_2 = dir_3;
            dir_3 = dir_4;
            dir_4 = dir;
        }
        else if (dir == dir_2) // Если движение назад более приоритетное
        {
            dir_2 = dir_3;
            dir_3 = dir_4;
            dir_4 = dir;
        }
        else if (dir == dir_3) // Если движение назад менее приоритетное
        {
            dir_3 = dir_4;
            dir_4 = dir;
        }
        // Иначе движение назад и так последнее в списке (dir_4)

        // В уровнях сложности до шестого использовать элемент случайности в выборе направления
        if ((uint8_t)rand() < bugs.swap)
        {
            // Поменять наиболее приоритетное направление с менее приоритетным
            dir_1 ^= dir_3;
            dir_3 ^= dir_1;
            dir_1 ^= dir_3;
        }

        if (bug->type == BUG_NOBBIN)
        {
            // Для Ноббинов нужно выбрать наилучшее направление по которому свободен путь
            const enum direction dirs[4] = { dir_1, dir_2, dir_3, dir_4 };
            for (uint8_t i = 0; i < 4; ++i)
            {
                if (check_path(dirs[i], bug_x_graph, bug_y_graph)) { dir = dirs[i]; break; }
            }
        }
        else
        {
            // Хоббины всегда идут по лучшему направлению, т.к. прокапывают себе путь
            dir = dir_1;
        }

        // Задержать врага на пересечении если он изменил направление движения
        if (bug->dir != dir) bug->wait++;

        bug->dir = dir; // Задать новое направление движения врага
    }

    // Для Хоббинов прокапывающих новый туннель
    if (bug->type == BUG_HOBBIN)
    {
        // Развернуть врага при попытке выхода за пределы экрана
        if (check_out_of_range(bug->dir, bug_x_graph, bug_y_graph))
        {
            bug->dir ^= 1; // Инвертировать направление движения
        }

        // Очистить биты фона прогрызенные Хоббином
        set_background_bits(bug_x_graph, bug_y_graph, bug->dir);
        plot_count++;

        // Стерерь кусочек фона на экране в соответствии с направлением движения и текущим положением
        gnaw(bug->dir, bug_x_graph, bug_y_graph);

        remove_coin(graph_to_x_log(bug_x_graph), graph_to_y_log(bug_y_graph));
    }

    if (man.state == CREATURE_ALIVE) // Если Диггер жив
    {
        // Выждать время задержки перед запуском нового врага: стоит на месте ровно count
        // вызовов, на следующем уже двигается (BUG_BEGIN в оригинале)
        if (bug->state == CREATURE_STARTING)
        {
            if (!--bug->count) bug->state = CREATURE_ALIVE; // Если счётчик закончился, то оживить врага
        }
        else
        {
            // Переместить врага на шаг в выбранном направлении.
            // MOVE_X_STEP=1, MOVE_Y_STEP=4 — складываются с шагом dir_dx/dir_dy.
            bug->x_graph += dir_dx[bug->dir] * MOVE_X_STEP;
            bug->y_graph += dir_dy[bug->dir] * MOVE_Y_STEP;
        }
    }

    if (bug->state == CREATURE_ALIVE)
    {
        uint8_t bag_hit = 0;  // Враг коснулся мешка или золота
        uint8_t ate_gold = 0; // Враг съел золото

        // Проверить соприкосновение врага с мешками
        for (uint8_t i = 0; i < MAX_BAGS; ++i)
        {
            struct bag_info *bag = &bags_state[i]; // Структура с информацией о мешках

            if (bag->state == BAG_INACTIVE) continue; // Пропустить неактивные мешки

            // Соприкосновение проверяется по новому положению врага
            if (check_collision_4_15(bag->x_graph, bag->y_graph, bug->x_graph, bug->y_graph))
            {
                bag_hit = 1;
                uint16_t remove_bag = 1; // Хоббин уничтожает мешок, а золото съедает любой враг

                if (bag->state == BAG_BROKEN)
                {
                    ate_gold = 1; // Враг съел золото из разбитого мешка
                }
                else if (bug->type == BUG_NOBBIN)
                {
                    remove_bag = 0;

                    // Если Ноббин коснулся мешка
                    switch (bag->state)
                    {
                        case BAG_STATIONARY: //  Если мешок стоит на месте
                        case BAG_LOOSE:      // Если мешок раскачивается
                        {
                            enum direction dir = bug->dir;

                            // Если Ноббин движется влево или вправо
                            if ((dir == DIR_UP) || (dir == DIR_DOWN) || move_bag(bag, dir))
                            {
                                // Если мешок не удалось подвинуть, отменить передвижение врага
                                bug->x_graph = bug_x_graph;
                                bug->y_graph = bug_y_graph;
                                bug->count++; // Увеличить счётчик застревания
                                plot_count++;

                                if ((dir == DIR_UP) || (dir == DIR_DOWN))
                                {
                                    bug->dir ^= 1; // Инвертировать направление движения врага
                                }

                                break;
                            }

                            break;
                        }
                    }
                }

                if (remove_bag)
                {
                    bag->state = BAG_INACTIVE; // Деактивировать мешок

                    // Стереть съеденный мешок или золото
                    sp_4_15_mask(bag->x_graph, bag->y_graph, nullptr, outline_bag_fall[0]);
                }
            }
        }

        // Как в оригинале: касание мешков задерживает врага на такт (один раз, сколько бы
        // мешков он ни задел), а толкание вбок - ещё на такт, даже если мешок сдвинулся.
        // Съевший золото враг не задерживается вовсе (rst_bwait в оригинале)
        if (ate_gold)
        {
            bug->wait = 0;
        }
        else if (bag_hit)
        {
            bug->wait++;
            if (bug->dir < DIR_UP) bug->wait++;
        }

        // Для Хоббинов увеличивать счётчик застревания для превращения в Ноббина по времени
        if (bug->type == BUG_HOBBIN)
        {
            if (bug->count < 100) bug->count++; // Увеличивать счётчик застревания для автоматического превращения в Ноббина
        }
    }

    // Если враг сдвинулся
    if ((bug->x_graph != bug_x_graph) || (bug->y_graph != bug_y_graph))
    {
        // Подтереть след врага с нужной стороны
        erase_trail(bug->dir, bug->x_graph, bug->y_graph);
    }

    plot_count++;
    next_image_phase(&bug->image_phase, &bug->image_phase_inc); // Переключить фазу изображения

    // Отрисовка спрайта врага
    if (bug->type == BUG_NOBBIN)
    {
        sp_4_15_put(bug->x_graph, bug->y_graph, (uint8_t *)image_nobbin + bug->image_phase);
    }
    else if (bug->dir == DIR_RIGHT)
    {
        sp_4_15_put(bug->x_graph, bug->y_graph, (uint8_t *)image_hobbin_right + bug->image_phase);
    }
    else
    {
        sp_4_15_h_mirror_put(bug->x_graph, bug->y_graph, (uint8_t *)image_hobbin_right + bug->image_phase);
    }
}

/**
 * @brief Остановка мешка
 *
 * @param bag - указатель на структуру с информацией о мешке
 */
static void stop_bag(struct bag_info *bag)
{
    plot_count++;
    // Если мешок пролетел больше одного этажа, то он будет разбит
    bag->state = (bag->count > 1) ? BAG_BREAKS : BAG_STATIONARY;
    bag->dir = DIR_STOP; // Остановить мешок
    bag->count = 0;

    // Уничтожить все мешки под тем, который остановился
    for (uint16_t i = 0; i < MAX_BAGS; ++i)
    {
        struct bag_info *another_bag = &bags_state[i];  // Структура с информацией о мешке
        if (another_bag == bag) continue; // Пропустить мешок обработка которого производится
        if (another_bag->state == BAG_INACTIVE) continue; // Пропустить неактивные мешки

        if (check_collision_4_15(bag->x_graph, bag->y_graph, another_bag->x_graph, another_bag->y_graph))
        {
            erase_4_15(another_bag->x_graph, another_bag->y_graph); // Стереть мешок
            another_bag->state = BAG_INACTIVE;
            snd.break_bag = 1;
        }
    }
}

/**
 * @brief Перерисовать покоящиеся мешки, попавшие в стёртый блок 4x15
 *
 * Стирание надгробного камня Диггера или трупа врага, погибшего под упавшим на него
 * мешком, затирает и сам мешок: они находятся в одной клетке. Стационарный мешок и
 * золото из разбитого мешка сами себя больше не перерисовывают, поэтому их надо
 * вернуть на экран (падающий и раскачивающийся мешки рисуются каждый кадр сами).
 *
 * @param x_graph - координата X стёртого блока
 * @param y_graph - координата Y стёртого блока
 */
static void redraw_bags(uint16_t x_graph, uint16_t y_graph)
{
    for (uint8_t i = 0; i < MAX_BAGS; ++i)
    {
        struct bag_info *bag = &bags_state[i]; // Структура с информацией о мешке

        if ((bag->state != BAG_STATIONARY) && (bag->state != BAG_BROKEN)) continue;

        // Проверить, что мешок попал в стёртый блок
        if (!check_collision_4_15(x_graph, y_graph, bag->x_graph, bag->y_graph)) continue;

        if (bag->state == BAG_STATIONARY)
        {
            draw_bag(bag->x_graph, bag->y_graph); // Мешок с золотом
        }
        else
        {
            // Последняя фаза анимации - золото из разбитого мешка
            sp_4_15_put(bag->x_graph, bag->y_graph, (uint8_t *)image_bag_broke[2]);
        }
    }
}

/**
 * @brief Подпрограмма отрисовки Диггера
 */
static void draw_man()
{
    uint8_t cab = !mis.flying && !mis.wait; // Флаг наличия "башенки"

    next_image_phase(&man.image_phase, &man.image_phase_inc); // Переключить фазу изображения

    uint16_t image_phase = man.image_phase + ((cab) ? 0 : 3 * FRAME_4_15);
    const uint8_t *image = ((man.dir < DIR_UP) ? (uint8_t *)image_digger_right : (uint8_t *)image_digger_up) + image_phase;

    if (man.dir == DIR_LEFT)
    {
        // Едет влево (отзеркаленный правый)
        sp_4_15_h_mirror_put(man.x_graph, man.y_graph, image);
    }
    else if (man.dir == DIR_RIGHT || man.dir == DIR_UP)
    {
        // Едет вправо или вверх (обычный спрайт)
        sp_4_15_put(man.x_graph, man.y_graph, image);
    }
    else
    {
        // Едет вниз (отзеркален горизонтально и вертикально)
        sp_4_15_hv_mirror_put(man.x_graph, man.y_graph, image);
    }
}

/**
 * @brief Удаление монеты по заданному логическому положению
 *
 * @param x_log - логическая координата по оси X
 * @param y_log - логическая координата по оси Y
 *
 * @return 1 - монета по заданным координатам удалена, 0 - монета по заданным координатам отсутствует
 */
static int remove_coin(uint8_t x_log, uint8_t y_log)
{
    if ((x_log >= W_MAX) || (y_log >= H_MAX)) return 0;

    uint16_t coin_mask = 1 << x_log;
    if (coins[y_log] & coin_mask)
    {
        coins[y_log] &= ~coin_mask; // Сбросить бит соответствующий съеденной монете
        plot_count++;

        // Стереть съеденную монету (драгоценный камешек)
        sp_put(FIELD_X_OFFSET + x_log * POS_X_STEP, FIELD_Y_OFFSET + y_log * POS_Y_STEP + COIN_Y_OFFSET,
               sizeof(outline_no_coin[0]), sizeof(outline_no_coin) / sizeof(outline_no_coin[0]), nullptr, (uint8_t *)outline_no_coin);

        // Проверить, что все монетки (камешки) съедены
        uint16_t level_done = 1;
        for (uint16_t i = 0; i < sizeof(coins) / sizeof(coins[0]); ++i)
        {
            if (coins[i]) { level_done = 0; break; }
        }

        if (level_done) snd.done = 1; // Если все камешки съедены, то включить звук окончания уровня

        return 1;
    }

    return 0;
}

/**
 * @brief Подпрограмма обработки звуковых эффектов
 */
static void sound_effect()
{
    if (snd.coin) // Звук съедания монеты (драгоценного камня)
    {
        static const uint16_t coin_periods[] = { C5, D5, E5, F5, G5, A5, B5, C6 };
        uint16_t period = coin_periods[snd.coin_note & 7];
        sound(period, 16);
        snd.coin--;
    }

    if (snd.fire)
    {
        snd.fire_period += snd.fire_period >> 2;
        if (snd.fire_period > 800) snd.fire = 0;
        else
        {
            uint16_t period = snd.fire_period + (rand() & (snd.fire_period >> 2));
            sound(period, 10);
        }
    }

    if (snd.explode)
    {
        snd.explode = 0;

        uint16_t explode_snd_period = 1500 / N; // Начальный период звука взрыва

        for (uint16_t i = 10; i != 0; --i)
        {
            explode_snd_period -= explode_snd_period >> 3;
            sound(explode_snd_period, 16);
        }
    }

    if (snd.loose) // Звук раскачивающегося мешка
    {
        static const uint16_t loose_periods[] = { 2500 / N, 3000 / N, 2500 / N, 2000 / N };
        static const uint16_t loose_durances[] = { 14, 12, 14, 18 };

        if (!(snd.loose_snd_phase & 1))
        {
            uint16_t index = snd.loose_snd_phase >> 1;
            sound(loose_periods[index], loose_durances[index]);
        }

        if (++snd.loose_snd_phase > 7) snd.loose_snd_phase = 0;
    }

    if (snd.fall) // Звук падающего мешка
    {
        snd.fall_snd_phase = ~snd.fall_snd_phase;

        if (snd.fall_snd_phase) sound(snd.fall_period / 32, 16);
        else snd.fall_period += 48;
    }

    if (snd.break_bag) // Звук разбивающегося мешка
    {
        sound(15000 / N, 10);
        snd.break_bag = 0;
    }

    if (snd.money) // Звук съедаемого золота
    {
        for (uint8_t k = 10; k && snd.money; --k)
        {
            uint16_t period = (snd.money & 1) ? snd.money_period_2 : snd.money_period_1;
            snd.money_period_1 += snd.money_period_1 >> 4;
            snd.money_period_2 -= snd.money_period_2 >> 4;
            sound(period, 14);
            snd.money--;
        }
    }

    if (snd.chase) // Звук включения бонус-режима
    {
        uint16_t durance = 40;
        snd.chase_flip = ~snd.chase_flip;
        if (snd.chase_flip) sound(1230 / N, durance);
        else sound(1513 / N, durance);
    }

    if (snd.done) // Звук завершения уровня
    {
        static const uint8_t done_periods[] = { C5, E5, G5, D5, F5, A5, E5, G5, B5, C6 };

        for (uint16_t i = 0; i < sizeof(done_periods) / sizeof(done_periods[0]); ++i)
        {
            uint8_t period = done_periods[i];
            uint16_t durance = (i == 9) ? 800 : 300;
            sound_vibrato(period, durance);
            delay_ms(2);
        }
    }

    if (snd.bug) // Звук съедаемого врага
    {
        for (uint8_t sounds = 10; sounds && snd.bug; )
        {
            if (snd.bug_c1)
            {
                uint8_t c1_lb = snd.bug_c1 & 3;
                if (c1_lb == 0) snd.bug_period_held = snd.bug_period;
                if (c1_lb == 2) snd.bug_period_held = snd.bug_period - (snd.bug_period >> 4);
                snd.bug_c1--;
                snd.bug_period -= snd.bug_period >> 4;
                sound(snd.bug_period_held / 16, 14);
                sounds--;
            }
            else
            {
                snd.bug_c2--;
                if (!snd.bug_c2) {
                    snd.bug = 0;
                    break;
                }
                snd.bug_period = 1600;
                snd.bug_c1 = 20;
            }
        }
    }

    if (snd.life) // Звук получения жизни
    {
        sound(14 + snd.life, 22);
        snd.life -= 2;
    }
}

/**
 * @brief Инициализация игры
 */
static void init_game()
{
    game.level = 1;      // Начальный уровень
    set_level_params();  // Начальный экран и уровень сложности
    game.lives = 3;      // Начальное количество жизней
    game.score = 0;      // Начальное количество очков
    bonus.life_score = BONUS_LIFE_SCORE;

    add_score(0);  // Печать начального количества очков (нули)
    print_lives(); // Вывод начального количества жизней
    init_level();  // Начальная инициализация уровня
}

/**
 * @brief Обработка появления и перемещения врагов (Ноббинов и Хоббинов)
 */
static void process_bugs()
{
    // Обработка появления врагов
    if (bugs.delay_counter > 0) --bugs.delay_counter; // Отсчёт времени до появления нового врага
    else if (man.state == CREATURE_ALIVE) // Если Диггер жив
    {
        // Счётчик перезаряжается только при рождении врага: пока его некуда выпустить,
        // он остаётся нулевым, и враг появляется сразу, как освободится место (как в оригинале)
        if ((bugs.created < bugs.total) && (bugs.active < bugs.max)) // Если врагов на экране одновременно меньше максимального количества
        {
            if (bonus.state != BONUS_ON) // Если не включен бонус-режим, запустить нового врага
            {
                for (uint16_t i = 0; i < bugs.max; ++i)
                {
                    struct bug_info *bug = &bugs_state[i];

                    if (bug->state != CREATURE_INACTIVE) continue; // Пропустить активных врагов

                    // Начальное состояние врага
                    bug->state = CREATURE_STARTING; // Враг стартует
                    bug->count = 5;                 // Время до запуска врага (BUG_BEGIN в оригинале)
                    bug->wait = 0;                  // Враг не задержан
                    bug->image_phase = 0;           // Начальная фаза отрисовки спрайта
                    bug->image_phase_inc = FRAME_4_15; // Начальное направление изменения фазы
                    bug->x_graph = CORNER_X;        // Начальная графическая координата по оси X (правый верхний угол)
                    bug->y_graph = CORNER_Y;        // Начальная графическая координата по оси Y
                    bug->type = BUG_NOBBIN;         // Враги рождаются в виде Ноббинов
                    // Как в оригинале - влево. DIR_STOP нельзя: если ни один путь не свободен,
                    // move_bug берёт обратное направление dir ^ 1 и вышел бы за таблицы dir_dx/dir_dy
                    bug->dir = DIR_LEFT;

                    bugs.active++;  // Увеличить счётчик активных врагов
                    bugs.created++; // Увеличить общее количество созданных врагов
                    bugs.delay_counter = bugs.delay; // Задержка до появления следующего врага

                    break;
                }
            }
        }
        else
        {
            // Если Бонус (вишенка) ещё не появлялся и создано максимальное количество врагов
            if ((bonus.state == BONUS_OFF) && (bugs.created == bugs.total))
            {
                bonus.state = BONUS_READY; // Включить готовность к активации бонус-режима
            }
        }
    }

    // Обработка врагов (Ноббинов и Хоббинов)
    for (uint16_t i = 0; i < bugs.max; ++i)
    {
        struct bug_info *bug = &bugs_state[i]; // Структура с информацией о враге

        if (bug->state == CREATURE_INACTIVE) continue; // Пропустить, если враг не активен

        switch (bug->state)
        {
            case CREATURE_ALIVE:    // Перемещение живого врага
            case CREATURE_STARTING: // Враг ждёт старта
            {
                // Если враг в режиме ожидания. Как в оригинале, ожидание (например, после выбора
                // направления на месте рождения) откладывает и отсчёт задержки старта
                if (bug->wait)
                {
                    bug->wait--; // Уменьшить счётчик в режиме ожидания
                    break;
                }

                if (bug->state == CREATURE_STARTING)
                {
                    move_bug(bug); // Отсчитать задержку старта
                    break;
                }

                // Столкновения с другими врагами (как dir_change/BUG_STUCK в оригинале): любое
                // столкновение задерживает врага, попутчика разворачивает, Ноббину копит застревание
                uint8_t collided = 0;
                for (uint16_t t = 0; t < bugs.max; ++t)
                {
                    if (t == i) continue; // Пропустить самого себя

                    struct bug_info *another_bug = &bugs_state[t];
                    if (another_bug->state == CREATURE_INACTIVE) continue; // Пропустить неактивных врагов

                    // Если враг соприкоснулся с другим врагом
                    if (check_collision_4_15(bug->x_graph, bug->y_graph, another_bug->x_graph, another_bug->y_graph))
                    {
                        collided = 1;
                        plot_count++;

                        // Если оба уже движутся в одном направлении - развернуть другого
                        if ((another_bug->state == CREATURE_ALIVE) && (bug->dir == another_bug->dir))
                        {
                            another_bug->dir ^= 1; // Инвертировать направление движения
                        }
                    }
                }

                if (collided)
                {
                    bug->wait++; // Увеличить счётчик ожидания
                    if ((bug->type == BUG_NOBBIN) && (man.state == CREATURE_ALIVE)) bug->count++; // Увеличить счётчик застревания
                }

                if (bug->type == BUG_NOBBIN) // Если это Ноббин
                {
                    //  Если Ноббин застрял или соприкоснулся с другим на определённое (зависящее от уровня сложности) время
                    if (bug->count > (10 - game.difficulty))
                    {
                        bug->count = 0;         // Сбросить счётчик застревания
                        bug->type = BUG_HOBBIN; // Переключить тип врага на Хоббина
                    }
                }

                // Лишний ход для увеличения скорости - только Ноббину, как в оригинале
                if ((bug->type == BUG_NOBBIN) && ((uint8_t)rand() < bugs.boost)) move_bug(bug);

                move_bug(bug); // Переместить врага
                break;
            }

            case CREATURE_DEAD_MONEY_BAG: // Враг погиб от мешка с деньгами
            {
                bug->count = 1;
                bug->state = CREATURE_RIP; // Враг лежит дохлый
                add_score_250(); // Добавить 250 очков за убитого мешком врага

                break;
            }

            case CREATURE_RIP: // Враг лежит дохлый
            {
                if (bug->count)
                {
                    bug->count--; // Уменьшить счётчик дохлого врага
                    break;
                }

                erase_4_15(bug->x_graph, bug->y_graph); // Стереть убитого врага
                redraw_bags(bug->x_graph, bug->y_graph); // Вернуть мешок, под которым погиб враг
                bug->state = CREATURE_INACTIVE;         // Деактивировать убитого врага
                bugs.active--;                          // Уменьшить количество активных врагов

                // Количество оставшихся врагов (сколько осталось создать плюс количество активных)
                uint8_t creatures_left =  bugs.total - bugs.created + bugs.active;
                if (!creatures_left) snd.done = 1; // Если врагов больше не осталось - окончание уровня

                break;
            }
        }
    }
}

/**
 * @brief Обработка мешков
 */
static void process_bags(const uint8_t man_x_log, const uint8_t man_y_log)
{
    // Обработка мешков
    uint8_t bags_fall = 0;  // Флаг, показывающий, что присутствуют падающие мешки
    uint8_t bags_loose = 0; // Флаг, показывающий, что присутствуют качающиеся мешки

    for (uint8_t i = 0; i < MAX_BAGS; ++i)
    {
        struct bag_info *bag = &bags_state[i]; // Структура с информацией о мешке

        uint16_t bag_x_graph = bag->x_graph;
        uint16_t bag_y_graph = bag->y_graph;
        uint8_t bag_x_log = graph_to_x_log(bag_x_graph);
        uint8_t bag_y_log = graph_to_y_log(bag_y_graph);
        uint8_t bag_x_rem = graph_to_x_rem(bag_x_graph);
        uint8_t bag_y_rem = graph_to_y_rem(bag_y_graph);

        switch (bag->state)
        {
            case BAG_INACTIVE: // Мешок неактивен
            {
                continue; // Пропустить неактивные мешки
            }

            case BAG_STATIONARY: // Мешок покоится на месте
            {
                if (bag_x_rem == 0) // Если мешок находится в серединге клетки игрового поля по-горизонтали
                {
                    // Если мешок не на самой нижней линии и клетка ниже повреждена
                    if ((bag_y_log != H_MAX - 1) && (background[bag_y_log + 1][bag_x_log] & 0x66)  )
                    {
                        switch (bag->dir)
                        {
                            case DIR_STOP: // Если мешок неподвижен
                            {
                                // Не раскачивать мешок, пока под ним (или в его клетке) Диггер, повёрнутый
                                // вверх или вниз - даже стоящий на месте (get_man_block в оригинале)
                                if (!((man_x_log == bag_x_log) &&
                                      ((man_y_log == bag_y_log + 1) || (man_y_log == bag_y_log)) &&
                                      ((man.dir == DIR_UP) || (man.dir == DIR_DOWN))))
                                {
                                    // Начать раскачивать мешок
                                    bag->state = BAG_LOOSE;  // Мешок раскачивается
                                    bag->count = LOOSE_WAIT; // Время раскачивания мешка
                                }

                                break;
                            }

                            case DIR_LEFT: // Если мешок движется всторону
                            case DIR_RIGHT:
                            {
                                // Если мешок сдвинули на повреждённую область, он проваливается
                                bag->state = BAG_FALLING; // Начать падение мешка
                                bag->dir = DIR_DOWN;      // Направление движения мешка вниз
                                bag->count = 0;           // Сбросить счётчик этажей
                                break;
                            }
                        }
                    }

                    // Остановить мешок. Начавшему падать мешку направление не сбрасывать:
                    // по DIR_DOWN обработчик гибели определяет, что мешок ещё летит, и Диггер
                    // падает вместе с ним, а не гибнет сразу (см. CREATURE_DEAD_MONEY_BAG).
                    if (bag->state != BAG_FALLING) bag->dir = DIR_STOP;
                }

                // Если мешок не остановлен (и не начал падать - падение обработается на следующем кадре)
                if ((bag->state == BAG_STATIONARY) && (bag->dir != DIR_STOP))
                {
                    move_bag(bag, bag->dir); // Перемещать мешок
                }

                break;
            }

            case BAG_LOOSE: // Мешок раскачивается
            {
                bags_loose = 1; // Найден качающийся мешок

                if (bag->count) // Если счётчик не закончился, мешок раскачивается
                {
                    if (!snd.loose) // Если звук раскачивания мешка ещё не включен
                    {
                        // Включить звук раскачивания мешка
                        snd.loose = 1;
                        snd.loose_snd_phase = 0;
                    }

                    bag->count--; // Уменьшить счётчик отсчитывающий время до падения мешка

                    uint8_t count_rem = bag->count & 7;
                    if ((count_rem & 1) == 0) // Каждый второй вызов рисовать анимацию раскачивающегося мешка
                    {
                        count_rem >>= 1;

                        // Порядок следования спрайтов и масок анимации раскачивающегося мешка
                        static uint8_t *bag_images[] = { (uint8_t *)image_bag_left, (uint8_t *)image_bag, (uint8_t *)image_bag_right, (uint8_t *)image_bag };
                        static uint8_t *bag_outlines[] = { (uint8_t *)outline_bag_left, (uint8_t *)outline_bag, (uint8_t *)outline_bag_right, (uint8_t *)outline_bag };

                        uint8_t *bag_image = bag_images[count_rem];     // Указатель на спрайт
                        uint8_t *bag_outline = bag_outlines[count_rem]; // Указатель на маску

                        // Нарисовать спрайт раскачивающегося мешка (используя маску)
                        sp_4_15_mask(bag_x_graph, bag_y_graph, bag_image, bag_outline);
                        plot_count++;
                    }
                }
                else
                {
                    // Если счётчик времени до падения мешка закончился
                    bag->state = BAG_FALLING; // Начать падение мешка
                    bag->dir = DIR_DOWN;      // Направление движения мешка - вниз
                    bag->count = 0;           // Сбросить счётчик этажей
                    // Звук падения (с инициализацией fall_period) включает проверка bags_fall
                    // ниже — см. коммент выше про зависание при падении двух мешков.
                }

                break;
            }

            case BAG_FALLING: // Мешок падает
            {
                bags_fall = 1; // Найден падающий мешок
                plot_count += 2;

                // Прогрызть фон и сбросить биты матрицы фона
                gnaw(DIR_UP, bag_x_graph, bag_y_graph + 9);
                set_background_bits(bag_x_graph, bag_y_graph, DIR_DOWN); // Сбросить биты матрицы фона
                set_background_bits(bag_x_graph, bag_y_graph - 1, DIR_DOWN); // Сбросить биты матрицы фона

                // Стереть падающий мешок по старым координатам
                if (bag->count) // Если номер этажа не нулевой
                {
                    // Если пролетел больше одного этажа, то стираем прямоугольником
                    erase_4_15(bag->x_graph, bag->y_graph);
                }
                else
                {
                    // В начале полёта стираем при помощи маски
                    sp_4_15_mask(bag->x_graph, bag->y_graph, nullptr, outline_bag_fall[0]);
                }

                // Перемещаем мешок в новое положение по оси Y
                bag_y_graph += 2 * MOVE_Y_STEP; // Скорость падения мешка вдвое выше скорости перемещения врагов
                bag_y_log = graph_to_y_log(bag_y_graph);
                bag_y_rem = graph_to_y_rem(bag_y_graph);

                if (bag_y_rem == 0) // Если мешок находится в центре клетки по-вертикали, значит он пролетел один этаж
                {
                    bag->count++; // Увеличить количество этажей, которые пролетел мешок

                    // Остановить мешок, если клетка под ним не повреждена или он долетел до
                    // последнего этажа. Порог тот же, что и для начала падения - маска 0x66
                    // (середина клетки). Прежняя проверка "клетка ненулевая" пропускала мешок
                    // этажом ниже: объект, движущийся вниз, ставит в клетке под собой бит
                    // прохода на подпозицию раньше, чем реально её прогрызёт, так что от
                    // прокопавшего сверху Диггера в целой клетке остаётся бит 0x10.
                    if ((bag_y_log == H_MAX - 1) || !(background[bag_y_log + 1][bag_x_log] & 0x66)) stop_bag(bag);
                }

                remove_coin(bag_x_log, bag_y_log); // Удалить монету в клетке куда попал мешок

                if (bag->state == BAG_FALLING)
                {
                    // Нарисовать падающий мешок
                    sp_4_15_put(bag_x_graph, bag_y_graph, (uint8_t *)image_bag_fall);
                    // sp_put(bag_x_graph, bag_y_graph, sizeof(image_bag_fall[0]), sizeof(image_bag_fall) / sizeof(image_bag_fall[0]),
                    //         (uint8_t *)image_bag_fall, (uint8_t *)outline_bag_fall);
                }
                else
                {
                    // Мешок только что остановлен процедурой stop_bag - вернуть ему вид стоящего
                    // на месте (иначе на экране так и остался бы спрайт падающего мешка).
                    // outline_bag перекрывает outline_bag_fall во всех строках, поэтому маска
                    // стирает остатки спрайта падающего мешка целиком.
                    // Для разбивающегося мешка (BAG_BREAKS) это кадр перед началом анимации,
                    // как и в оригинале (stop_bag там рисует стационарный мешок).
                    draw_bag(bag_x_graph, bag_y_graph);
                }

                if (man.state == CREATURE_ALIVE) //  Если Диггер жив
                {
                    // Проверить, что Диггер попал под падающий под мешок
                    if (check_collision_4_15(man.x_graph, man.y_graph, bag_x_graph, bag_y_graph))
                    {
                        man.state = CREATURE_DEAD_MONEY_BAG; // Диггер погиб от падающего мешка
                        man.dead_bag = bag; // Указатель на мешок от которого погиб Диггер
                    }
                }

                // Попытаться спасти врагов от падающего мешка
                for (uint8_t i = 0; i < bugs.max; ++i)
                {
                    struct bug_info *bug = &bugs_state[i]; // Структура с информацией о враге

                    // Пропустить неживых врагов. Стоящий на месте рождения враг
                    // (CREATURE_STARTING) гибнет под мешком наравне с живым
                    if ((bug->state != CREATURE_ALIVE) && (bug->state != CREATURE_STARTING)) continue;

                    uint16_t bug_x_graph = bug->x_graph;

                    // Сделать чтобы враги пытались убежать от летящего мешка
                    // Если враг находится на одной вертикальной линии с мешком и
                    // движется вверх, то изменить направление движения на движение вниз
                    if (graph_to_x_log(bug_x_graph) == bag_x_log && bug->dir == DIR_UP) bug->dir = DIR_DOWN;

                    // Проверить, что враг попал под падающий мешок
                    if (check_collision_4_15(bug->x_graph, bug->y_graph, bag_x_graph, bag_y_graph + 8))
                    {
                        bug->state = CREATURE_DEAD_MONEY_BAG; // Враг был убит мешком с деньгами

                        // В бонус-режиме увеличить количество создаваемых врагов компенсируя убитых мешками.
                        if (bonus.state == BONUS_ON) bugs.total++;
                    }
                }

                bag->y_graph = bag_y_graph;

                break;
            }

            case BAG_BREAKS: // Мешок разбивается
            {
                volatile uint16_t *v_scroll = (volatile uint16_t *)REG_V_SCROLL;

                // Анимация разбивающегося мешка (три фазы, пропуская один такт счётчика)
                if (bag->count++ < 6)
                {
                    if (bag->count == 1)
                    {
                        // Первый шаг анимации
                        *v_scroll = 0327 | (1 << V_SCROLL_EXT_MEMORY); // Экран "проваливается"
                        snd.break_bag = 1; // Издать звук разбившегося мешка
                    }
                    else
                    {
                        *v_scroll = 0330 | (1 << V_SCROLL_EXT_MEMORY); // Восстановить положение экрана
                    }

                    if (bag->count & 1)
                    {
                        // Смещения кадров таблицей: индекс пришлось бы умножать вызовом __mulhi3
                        static const uint8_t broke_frames[3] = { 0, FRAME_4_15, 2 * FRAME_4_15 };

                        // Нарисовать анимацию рассыпающегося золота
                        sp_4_15_put(bag->x_graph, bag->y_graph, (uint8_t *)image_bag_broke + broke_frames[(bag->count - 1) >> 1]);
                        plot_count++;
                    }
                }
                else
                {
                    bag->state = BAG_BROKEN; // Мешок разбился
                    bag->count = 0; // Сбросить счётчик существования разбившегося мешка
                }

                break;
            }

            case BAG_BROKEN:  // Если мешок разбит
            {
                bag->count++; //  Увеличить счётчик существования разбившегося мешка

                if (bag->count >= broke_max) // Если время существования разбившегося мешка достигло максимального
                {
                    bag->state = BAG_INACTIVE; // Сделать мешок неактивным

                    // Стереть разбившийся мешок
                    erase_4_15(bag->x_graph, bag->y_graph);
                }
                else if ((bag_y_log < H_MAX - 1) && (bag->count < broke_max - 10) &&
                         full_bite(background[bag_y_log + 1][bag_x_log]))
                {
                    // Под золотом прокопан проход - оно исчезает быстрее (как в оригинале)
                    bag->count = broke_max - 10;
                }

                break;
            }

        }
    }

    if (snd.fall) // Если звук падения мешков включен
    {
        if (!bags_fall) // Но, мешков падающих нет
        {
            snd.fall = 0; // Выключить звук падения мешков
        }
    }
    else
    {
        if (bags_fall) // Если есть падающие мешки
        {
            // Включить звук падения мешков
            snd.fall_period = 1024;
            snd.fall_snd_phase = 0;
            snd.fall = 1;
        }
    }

    if (!bags_loose) // Если ни один мешок не качается
    {
        snd.loose = 0; // Выключить звук качающегося мешка
    }
}

/**
 * @brief Обработка выстрела
 */
static void process_missile()
{
    // Размеры и количество фаз анимации выстрела
    constexpr uint16_t missile_x_size = sizeof(image_missile[0][0]);
    constexpr uint16_t missile_y_size = sizeof(image_missile[0]) / missile_x_size;

    // Размеры и количество фаз анимации взрыва
    constexpr uint16_t explode_x_size = sizeof(image_explode[0][0]);
    constexpr uint16_t explode_y_size = sizeof(image_explode[0]) / explode_x_size;

    if (mis.explode)
    {
        // Обработка взрывающегося выстрела
        if (mis.image_phase < sizeof(image_explode))
        {
            // Вывести изображение взрыва
            sp_put(mis.x_graph, mis.y_graph, explode_x_size, explode_y_size, (uint8_t *)image_explode + mis.image_phase, nullptr);
            mis.image_phase += sizeof(image_explode[0]);
            plot_count++;
        }
        else
        {
            // Стереть изображение взрыва
            sp_clear_brick(mis.x_graph, mis.y_graph, explode_x_size, explode_y_size);
            mis.flying = 0;  // Выстрел больше не летит
            mis.explode = 0; // И не взрывается
        }
    }
    else
    {
        // Обработка летящего выстрела
        if (mis.flying)
        {
            // Стереть предыдущее изображение выстрела
            // sp_put(mis.x_graph, mis.y_graph, missile_x_size, missile_y_size, nullptr, (uint8_t *)outline_missile);
            sp_clear_brick(mis.x_graph, mis.y_graph, missile_x_size, missile_y_size);

            // Переместить выстрел на один шаг в заданном направлении.
            // Скорость выстрела вдвое выше скорости перемещения врагов и Диггера.
            mis.x_graph += dir_dx[mis.dir] * (2 * MOVE_X_STEP);
            mis.y_graph += dir_dy[mis.dir] * (2 * MOVE_Y_STEP);

            uint8_t explode = 0;

            // Проверить если координаты выходят за рамки игрового поля или впереди нету прохода

            // Проверить попал ли выстрел во врага. Делается ДО check_path,
            // иначе если снаряд достиг последней клетки тоннеля одновременно с врагом
            // (стена впереди), check_path фейлит и враг остаётся живым.
            for (uint8_t i = 0; i < bugs.max; ++i)
            {
                struct bug_info *bug = &bugs_state[i]; // Структура с информацией о враге

                // Пропустить неживых врагов. Враг, ещё стоящий на месте рождения
                // (CREATURE_STARTING), уязвим наравне с живым - он и убивает наравне с ним
                if ((bug->state != CREATURE_ALIVE) && (bug->state != CREATURE_STARTING)) continue;

                // Проверить, что выстрел попал во врага
                if (check_collision_missile(mis.x_graph, mis.y_graph, bug->x_graph, bug->y_graph))
                {
                    explode = 1; // Взорвать выстрел
                    bug->count = 1; // Чтобы CREATURE_RIP стёр врага на следующем тике, а не ждал старого счётчика
                    bug->state = CREATURE_RIP; // Враг был убит выстрелом
                    add_score_250(); // Добавить 250 очков за убитого врага

                    // В бонус-режиме увеличить количество создаваемых врагов компенсируя убитых выстрелом.
                    if (bonus.state == BONUS_ON) bugs.total++;
                }
            }

            if (!check_path(mis.dir, mis.x_graph, mis.y_graph))
            {
                explode = 1; // Взорвать выстрел - впереди стена/край
            }
            else if (!explode)
            {
                plot_count++;
                // Циклически менять фазу анимации выстрела
                mis.image_phase += sizeof(image_missile[0]);
                if (mis.image_phase >= sizeof(image_missile)) mis.image_phase = 0;

                // Вывести новое изображение выстрела
                sp_put(mis.x_graph, mis.y_graph, missile_x_size, missile_y_size, (uint8_t *)image_missile + mis.image_phase, nullptr);
            }

            if (explode)
            {
                snd.fire = 0;                  // Выключить звук летящего выстрела
                mis.explode = 1;               // Включить взрыв выстрела
                mis.image_phase = 0;           // Начальная фаза взрыва
                snd.explode = 1;               // Включить звук взрыва
            }
        }
        else
        {
            if (mis.wait) mis.wait--; // Уменьшить счётчик задержки выстрела
            else
            {
                if (mis.fire) // Если произведён выстрел
                {
                    mis.fire = 0;
                    mis.wait = 60 + mul3(game.difficulty); // Начальное значение счётчика появления "башенки" (как в оригинале)
                    mis.image_phase = 0;
                    mis.flying = 1;
                    mis.dir = man.dir;

                    // Определить начальное положение выстрела в зависимости от
                    // координат Диггера и его направления движения.
                    // Смещения нерегулярные (специфичные точки рождения снаряда
                    // относительно спрайта Диггера), общую таблицу dir_d* не используем.
                    //
                    // Снаряд рождается вплотную к спрайту Диггера (4x15), выступая за его
                    // край ровно на шаг перемещения: спрайт снаряда одним байтом (строкой)
                    // перекрывает Диггера. Если родить снаряд дальше - на клетку впереди, -
                    // то выстрел в упор во врага, бегущего в ту же сторону, промахивается:
                    // скорость снаряда всего вдвое выше скорости врага, и снаряд летит
                    // рядом с ним, ни разу не попав в зону проверки соприкосновения.
                    static const int8_t fire_dx[4] = {
                        -MOVE_X_STEP, 4 - missile_x_size + MOVE_X_STEP, 1, 1
                    };
                    static const int8_t fire_dy[4] = {
                        MOVE_Y_STEP, MOVE_Y_STEP, -MOVE_Y_STEP, 15 - missile_y_size + MOVE_Y_STEP
                    };

                    mis.x_graph = man.x_graph + fire_dx[mis.dir];
                    mis.y_graph = man.y_graph + fire_dy[mis.dir];

                    // Вывести начальное положение спрайта выстрела
                    sp_put(mis.x_graph, mis.y_graph, missile_x_size, missile_y_size, (uint8_t *)image_missile + mis.image_phase, nullptr);

                    // Включить звук выстрела
                    snd.fire_period = 10;
                    snd.fire = 1;
                }
            }
        }
    }
}

static void man_rip();

/**
 * @brief Съесть монету (драгоценный камень)
 */
static inline void eat_coin()
{
    snd.coin = 7;  // Включить звук съедения монеты
    snd.coin_time = 9; // Взвести таймер до последующего съедения монеты
    add_score(25); // 25 очков за съеденную монету (камешек)
    if (++snd.coin_note == 7) // Перейти к следующей ноте
    {
        snd.coin_note = -1;
        add_score_250(); // Добавить 250 очков за съедение восьми последовательных монет
    }
}

/**
 * @brief Обработка Диггера
 */
static void process_man()
{
    // Обработка перемещения Диггера
    if (man.state == CREATURE_ALIVE) // Если Диггер жив
    {
        // Положение Диггера на момент входа: по остаткам видно, стоит ли он
        // на границе клетки и можно ли менять направление движения
        const uint8_t man_x_rem = graph_to_x_rem(man.x_graph);
        const uint8_t man_y_rem = graph_to_y_rem(man.y_graph);

        plot_count++; // Диггер перерисовывается и в такт задержки

        if (man.wait) man.wait--; // Если Диггер в режиме задержки (при толкании мешков)
        else
        {
            man.new_dir = DIR_STOP;

            // Обработка управления с клавиатуры и джойстика
            const uint16_t port_state = *((volatile uint16_t *)REG_PAR_INTERF); // Состояние регистра параллельного порта
            // print_dec(port_state, 0, MAX_Y_POS + 2 * POS_Y_STEP);
            const uint8_t key_pressed = !(((union EXT_DEV *)REG_EXT_DEV)->bits.MAG_KEY);
            // Состояние читается до данных: чтение регистра данных сбрасывает флаг готовности
            const uint8_t new_code = (*(volatile uint8_t *)REG_KEY_STATE) & (1 << KEY_STATE_STATE);
            const uint8_t code = *((volatile uint8_t *)REG_KEY_DATA); // Скан-код нажатой клавиши
            if (key_pressed || port_state) // Если удерживают клавишу на клавиатуре или направление на джойстике
            {
                static const enum direction joy_dirs[] = { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT };
                static const uint8_t key_codes[] = { 26, 25, 27, 8 };

                // Раскладка направлений манипулятора "Электроника"
                for (uint16_t i = 0; i < sizeof(joy_dirs); ++i)
                {
                    if ((key_pressed && (code == key_codes[i])) || (port_state & (1 << i)))
                    {
                        man.new_dir = joy_dirs[i];
                        break;
                    }
                }
            }

            if (!mis.wait && (port_state & JOY_BUTTONS)) mis.fire = 1;

            if (new_code) // Если поступил новый скан-код
            {
                switch (code)
                {
                    case 12:  // СБР - Пауза
                    {
                        volatile uint16_t *joy = (volatile uint16_t *)REG_PAR_INTERF;
                        volatile uint8_t *key_state = (volatile uint8_t *)REG_KEY_STATE;

                        // Дождаться отпускания кнопки джойстика, иначе удерживаемая
                        // кнопка огня сняла бы паузу в тот же момент
                        while (*joy & JOY_BUTTONS);

                        // Пауза снимается любой клавишей или кнопкой джойстика
                        while (!(*key_state & (1 << KEY_STATE_STATE)) && !(*joy & JOY_BUTTONS));

                        // Забрать код клавиши, снявшей паузу, иначе на следующем кадре
                        // он будет обработан как команда (СБР снова поставит паузу)
                        (void)*(volatile uint8_t *)REG_KEY_DATA;

                        // Дождаться отпускания кнопки, чтобы снятие паузы не обернулось выстрелом
                        while (*joy & JOY_BUTTONS);

                        break;
                    }

                    case 32:  // Пробел - выстрел
                    {
                        if (!mis.wait) mis.fire = 1;
                        break;
                    }

                    case 'S':  // Переключить состояние звуковых эффектов
                    {
                        snd_effects = !snd_effects;
                        break;
                    }

                    case 'M':  // Переключить фоновую музыку
                    {
                        music_on = !music_on;
                        break;
                    }
#ifdef DEBUG
                    case 'D': // Увеличение уровня сложности
                    {
                        if (++game.difficulty > 10) game.difficulty = 1;
                        break;
                    }

                    case 'L':  // Добавление жизни
                    {
                        game.lives++;
                        print_lives();
                        snd.life = 24;
                        break;
                    }

                    case 'N':  // Переход на следующий уровень
                    {
                        snd.done = 1;
                        break;
                    }
#endif
                }
            }

            // Если новое желаемое направление движения вверх-вниз, то применить его в середине клетки по-горизонтали
            if (man_x_rem == 0 && (man.new_dir == DIR_UP || man.new_dir == DIR_DOWN))
            {
                man.dir = man.new_dir;
            }

            // Если новое желаемое направление движения влево-вправо, то применить его в середине клетки по-вертикали
            if (man_y_rem == 0 && (man.new_dir == DIR_LEFT || man.new_dir == DIR_RIGHT))
            {
                man.dir = man.new_dir;
            }

            // Остановиться при попытке выхода за игровое поле
            if ((man.new_dir == DIR_STOP) || check_out_of_range(man.dir, man.x_graph, man.y_graph))
            {
                man.dir = DIR_STOP;
            }

            if (bonus.state == BONUS_READY)
            {
                // Проверить что Диггер соприкоснулся с вишенкой
                if (check_collision_4_15(man.x_graph, man.y_graph, CORNER_X, CORNER_Y))
                {
                    bonus.state = BONUS_ON; // Включить Бонус-режим
                    bonus.count = 200; // Очки за первого пойманного в Бонус-режиме врага
                    bonus.time = 250 - mul5(game.difficulty << 2); // Время действия Бонус-режима
                    bonus.flash = 19; // Время мигания индикатора включения Бонус-режима

                    add_score(1000); // 1000 очков за вишенку

                    // Стереть вишенку
                    erase_4_15(CORNER_X, CORNER_Y);
                }
                else
                {
                    // Нарисовать вишенку в правом верхнем углу игрового поля
                    sp_4_15_put(CORNER_X, CORNER_Y, (uint8_t *)image_cherry);
                }
            }

            uint16_t prev_man_x_graph = man.x_graph;
            uint16_t prev_man_y_graph = man.y_graph;

            // Бит 0 - Диггер упёрся в мешок, бит 1 - Диггер стоит на месте
            uint16_t collision_flag = 0;

            if (man.dir != DIR_STOP)
            {
                plot_count++;

                // Переместить Диггера на один шаг в заданном направлении
                man.x_graph += dir_dx[man.dir] * MOVE_X_STEP;
                man.y_graph += dir_dy[man.dir] * MOVE_Y_STEP;
            }
            else
            {
                man.dir = man.prev_dir;
                collision_flag = 2;
            }

            // Обработка толкания мешков и съедения золота
            for (uint8_t i = 0; i < MAX_BAGS; ++i)
            {
                struct bag_info *bag = &bags_state[i]; // Структура с информацией о мешке
                if (bag->state == BAG_INACTIVE) continue; // Пропустить неактивные мешки

                // Если Диггер не соприкоснулся с мешком, проверить следующий мешок
                if (!check_collision_4_15(bag->x_graph, bag->y_graph, man.x_graph, man.y_graph)) continue;

                man.wait++; // Задержать Диггера перед мешком или золотом

                switch (bag->state)
                {
                    case BAG_STATIONARY:
                    case BAG_LOOSE:
                    {
                        // Если направление движения Диггера вверх или вниз, или мешок не удалось переместить
                        if (man.dir == DIR_UP || man.dir == DIR_DOWN || move_bag(bag, man.dir))
                        {
                            collision_flag |= 1;
                        }

                        break;
                    }

                    case BAG_FALLING:
                    {
                        // Как в оригинале: сверху или снизу в падающий мешок не пройти, а вбок он
                        // Диггера не останавливает. Убьёт его мешок в process_bags этого же кадра
                        if (man.dir >= DIR_UP) collision_flag |= 1;
                        break;
                    }

                    case BAG_BREAKS:
                    case BAG_BROKEN:
                    {
                        // Включить звук съедаемого золота
                        snd.money_period_1 = 500 / N; // Начальный период чётных звуков
                        snd.money_period_2 = 4000 / N; // Начальный период нечётных звуков
                        snd.money = 30; // Количество звуков в последовательности
                        plot_count++;
                        bag->state = BAG_INACTIVE; // Сделать мешок неактивным
                        add_score(500); // 500 очков за съеденное золото
                        // Стереть золото из разбитого мешка
                        erase_4_15(bag->x_graph, bag->y_graph);
                        break;
                    }
                }
            }

            if (collision_flag & 1)
            {
                plot_count++;

                // Вернуть Диггера в прежнее положение
                man.x_graph = prev_man_x_graph;
                man.y_graph = prev_man_y_graph;
            }
            else
            {
                if (man.x_graph != prev_man_x_graph || man.y_graph != prev_man_y_graph) // Если Диггер переместился
                {
                     // Очистить биты фона, который был "прогрызен"
                    set_background_bits(man.x_graph, man.y_graph, man.dir);
                    set_background_bits(man.x_graph, man.y_graph, man.dir ^ 1);

                    // Стереть след от Диггера с нужной стороы
                    erase_trail(man.dir, man.x_graph, man.y_graph);

                    // Нарисовать "прогрыз" от движения Диггера
                    gnaw(man.dir, prev_man_x_graph, prev_man_y_graph);

                    // Удалить монеты съеденные Диггером.
                    {
                        const uint8_t cx0 = graph_to_x_log(man.x_graph);
                        const uint8_t cy0 = graph_to_y_log(man.y_graph);
                        const uint8_t cx1 = graph_to_x_log(man.x_graph + 3);
                        const uint8_t cy1 = graph_to_y_log(man.y_graph + 11);
                        if (remove_coin(cx0, cy0)) eat_coin();
                        if (cx1 != cx0 && remove_coin(cx1, cy0)) eat_coin();
                        if (cy1 != cy0 && remove_coin(cx0, cy1)) eat_coin();
                        if (cx1 != cx0 && cy1 != cy0 && remove_coin(cx1, cy1)) eat_coin();
                    }
                }
            }

            draw_man(); // Нарисовать Диггера

            // Проверить Диггера на сопркосновение с врагами
            for (uint8_t i = 0; i < bugs.max; ++i)
            {
                struct bug_info *bug = &bugs_state[i]; // Структура с информацией о враге

                // Пропустить неактивных и дохлых врагов. Враг, ещё стоящий на месте
                // рождения (CREATURE_STARTING), опасен наравне с живым: в оригинале он
                // сразу помечается живым, а счётчик задержки лишь не даёт ему двигаться.
                if ((bug->state != CREATURE_ALIVE) && (bug->state != CREATURE_STARTING)) continue;

                // Если Диггер не касается врага
                if (!check_collision_4_15(bug->x_graph, bug->y_graph, man.x_graph, man.y_graph)) continue;

                if (bonus.state == BONUS_ON)
                {
                    // Если включен режим Бонус
                    snd.bug = 1; // Запустить воспроизведение звука съедения врага
                    snd.bug_c1 = 0;
                    snd.bug_c2 = 4;
                    snd.bug_period = 0;
                    add_score(bonus.count); // 200, 400, 800... очков за каждого съеденного врага
                    bonus.count <<= 1; // Удвоить очки за следующего

                    // Стереть съеденного врага
                    erase_4_15(bug->x_graph, bug->y_graph);
                    bug->state = CREATURE_INACTIVE; // Деактивировать врага

                    bugs.active--; // Уменьшить количество активных врагов
                    bugs.total++;  // Увеличить количество создаваемых врагов компенсируя съеденных
                }
                else
                {
                    erase_4_15(man.x_graph, man.y_graph); // Стереть Диггера
                    man_rip();
                }
            }

            // Как в оригинале: Диггер, упёршийся в мешок на ходу, разворачивается. Иначе, пока
            // он не дошёл до середины клетки и держат поперечное направление, он бы так
            // и бился в мешок. Разворот после отрисовки: нарисован он лицом к мешку
            if (collision_flag == 1) man.dir ^= 1;
            man.prev_dir = man.dir;
        }
    }
    else
    {
        // Перемещение и отрисовка убитого Диггера
        if (man.state == CREATURE_DEAD_MONEY_BAG)
        {
            uint8_t bag_y_pos = man.dead_bag->y_graph; // Вертикальная позиция мешка от которого погиб Диггер
            if (bag_y_pos > man.y_graph)
            {
                erase_4_15(man.x_graph, man.y_graph);
                man.y_graph = bag_y_pos; // Если мешок опустился ниже Диггера, Диггер перемещается за мешком
            }

            // Нарисовать перевёрнутого Диггера
            sp_4_15_mask(man.x_graph, man.y_graph, image_digger_turned_over[0], outline_digger_turned_over[0]);

            // Мешок остановился, либо его убрали в полёте (съел Хоббин, раздавил другой мешок) -
            // проверка по dir == DIR_STOP во втором случае подвесила бы игру с мёртвым Диггером
            if (man.dead_bag->state != BAG_FALLING)
            {
                man_rip();
            }
        }
    }
}

/**
 * @brief Подпрограмма анимации гибели Диггера, отрисовки надгробного камня и
 *        воспроизведения музыкального сопровождения
 */
static void man_rip()
{
    // Последовательность высоты на которую подпрыгивает перевёрнутый Диггер
    static uint8_t bounce[8] = { 3, 5, 6, 6, 5, 4, 3, 0 };

    reset_v_scroll(); // Чтобы анимация гибели не проигрывалась на "провалившемся" экране

    // Убрать летящий снаряд (или его взрыв). Анимация гибели блокирующая, поэтому
    // снаряд провисел бы на экране всё это время, после неё сделал бы последний шаг
    // со сменой фазы, а дальше init_level_state сбросил бы mis.flying - и стирать
    // спрайт стало бы уже некому.
    if (mis.flying)
    {
        sp_clear_brick(mis.x_graph, mis.y_graph, sizeof(image_explode[0][0]),
                       sizeof(image_explode[0]) / sizeof(image_explode[0][0]));
        mis.flying = 0;
        mis.explode = 0;
    }

    mis.fire = 0; // Не рождать новый снаряд у погибшего Диггера

    uint16_t prev_y_graph = 0;
    uint16_t period = 19000 / N;
    uint16_t i = 0;
    while (period < 36000 / N) // Звук убиения Диггера
    {
        if (snd_effects) sound(period, 2);

        if (man.state != CREATURE_DEAD_MONEY_BAG)
        {
            uint16_t y_graph = man.y_graph - bounce[i >> 3];

            // Анимация подпрыгивающего перевёрнутого Диггера
            if (prev_y_graph)
            {
                sp_4_15_mask(man.x_graph, prev_y_graph, nullptr, outline_digger_turned_over[0]);
            }

            sp_4_15_mask(man.x_graph, y_graph, image_digger_turned_over[0], outline_digger_turned_over[0]);

            prev_y_graph = y_graph;
        }
        else
        {
            delay_ms(10);
        }

        if (i++ < 10)
        {
            period -= 1000 / N;
        }
        else
        {
            period += 500 / N;
        }
    }

    delay_ms(500);

    for (uint8_t i = 0; i < bugs.max; ++i)
    {
        struct bug_info *bug = &bugs_state[i]; // Структура с информацией о враге
        if (bug->state == CREATURE_INACTIVE) continue; // Пропустить неактивных врагов

        // Проверить, что враг оказался рядом с могилкой
        if (check_collision_4_15(man.x_graph, man.y_graph, bug->x_graph, bug->y_graph))
        {
            bug->state = CREATURE_INACTIVE; // Деактивировать врага убившего Диггера
            erase_4_15(bug->x_graph, bug->y_graph); // Стереть деактивированного врага
            redraw_bags(bug->x_graph, bug->y_graph); // Вернуть задетый стиранием мешок
        }
    }

    // Траурный марш
    static const uint8_t music_dead_periods[]   = { C4, C4, C4, C4, DS4, D4, D4, C4, C4, B3, C4 };
    static const uint16_t music_dead_durations[] = { N6, NQ, NE, N6, NQ, NE, NQ, NE, NQ, NE, N12 };

    // Фазы отрисовки надгробного камня
    static const uint8_t rip_frames[5][2] = {
        { 10,  4 }, // Верхушка надгробия только показалась
        {  8,  6 },
        {  6,  8 },
        {  3, 11 },
        {  1, 14 }, // Полный надгробный камень
    };

    for (uint16_t i = 0; i < sizeof(music_dead_periods)/ sizeof(music_dead_periods[0]); ++i)
    {
        uint8_t period = music_dead_periods[i];
        uint16_t duration = music_dead_durations[i];
        if (snd_effects) sound_vibrato(period, duration);

        if (i < sizeof(rip_frames) / sizeof(rip_frames[0]))
        {
            sp_put(man.x_graph, man.y_graph + rip_frames[i][0], 4, rip_frames[i][1],
                   (uint8_t *)image_rip, nullptr);
        }

        delay_ms(30);
    }

    erase_4_15(man.x_graph, man.y_graph); // Стереть надгробный камень
    redraw_bags(man.x_graph, man.y_graph); // Вернуть мешок, под которым погиб Диггер

    (void)*(volatile uint8_t *)REG_KEY_DATA;

    man.state = CREATURE_RIP;
}

__attribute__((noinline)) static void bonus_indicator(uint16_t color)
{
    volatile uint16_t *ptr_up = (uint16_t *)MEM_VIDEO;
    volatile uint16_t *ptr_down = (uint16_t *)MEM_VIDEO + SCREEN_WORD_WIDTH * SCREEN_PIX_HEIGHT - 1;
    for (uint16_t i = 0; i < SCREEN_WORD_WIDTH * SCREEN_Y_OFFSET; ++i)
    {
        *ptr_down-- = *ptr_up++ = color;
    }
}

/**
 * @brief Обработка бонуса
 */
static void process_bonus()
{
    // Обработка Бонус-режима
    if (bonus.state == BONUS_ON) // Если включен Бонус-режим
    {
        if ((man.state == CREATURE_ALIVE) && bonus.time) // Если Диггер жив и время Бонус-режима не закончилось
        {
            bonus.time--; // Декрементировать время Бонус-режима

            // Мигание в начале и в конце времени Бонус-режима
            if (bonus.flash || bonus.time < 20)
            {
                bonus.flash--;
                snd.chase = bonus.flash;

                // Мигание при включении закончилось - переключиться на музыку бонус-режима.
                // Срабатывает ровно один раз: на финальном мигании счётчик уходит в 255.
                if (bonus.flash == 0) bg_music_track(BG_MUSIC_BONUS);

                // Мигание при включении бонус-режима
                bonus_indicator((bonus.time & 1) ? (bonus.flash ? 0xFFFF : 0xAAAA) : 0x0000);
            }
        }
        else
        {
            bonus.state = BONUS_END;
            snd.chase = 0; // Выключить звук включения/выключения бонус-режима
            bugs.delay_counter = 0; // Враги начинают появляться сразу же после окончания бонус-режима

            bg_music_track(BG_MUSIC_POPCORN); // Вернуть музыку Popcorn
        }
    }
}

/**
 * @brief Обработка общего состояния игры (переход на новый уровень, Game Over и т.д.)
 */
static void process_game_state()
{
    // Декрементировать таймер между последовательными съедениями драгоценных камней (монеток)
    if (snd.coin_time > 0) snd.coin_time--;
    else snd.coin_note = -1;

    if (snd.done)
    {
        snd.done = 0;

        // Переход на следующий уровень (номер уровня клампится на 1000, как в оригинале)
        if (game.level < 1000) game.level++;

        // Экран и уровень сложности нового уровня
        set_level_params();

        init_level(); // Инициализация нового уровня
    }

    if (man.state == CREATURE_RIP)
    {
        game.lives--;  // Уменьшить количество жизней
        print_lives(); // Вывести количество жизней

        if (game.lives > 0) // Проверить остались ли ещё жизни
        {
            // Если жизни остались
            init_level_state(); // Инициализировать состояние уровня
            man.state = CREATURE_ALIVE; // Оживить Диггера
        }
        else
        {
            constexpr uint16_t go_width = sizeof(game_over[0]);
            constexpr uint16_t go_height = sizeof(game_over) / go_width;
            constexpr uint16_t go_x = (SCREEN_BYTE_WIDTH - go_width) / 2;
            constexpr uint16_t go_y = (SCREEN_PIX_HEIGHT - go_height) / 2;
            constexpr uint16_t f_x = go_x - 2 * MOVE_X_STEP;
            constexpr uint16_t f_y = go_y - (MOVE_Y_STEP + 2);
            constexpr uint16_t f_w = go_width + 4 * MOVE_X_STEP;
            constexpr uint16_t f_h = go_height + 2 * (MOVE_Y_STEP + 2);
            sp_clear_brick(f_x, f_y, f_w, f_h); // Очистка фона для надписи Game Over
            volatile uint8_t *p = (volatile uint8_t *)MEM_VIDEO + f_y * SCREEN_BYTE_WIDTH + f_x;
            volatile uint8_t *q = p + (f_h - 1) * SCREEN_BYTE_WIDTH;
            // Отрисовка рамки вокруг надписи Game Over
            for (uint16_t i = 0; i < f_w; i++) p[i] = p[i + SCREEN_BYTE_WIDTH] = q[i] = q[i - SCREEN_BYTE_WIDTH] = 0xFF;
            p += 2 * SCREEN_BYTE_WIDTH;
            for (uint16_t i = 0; i < f_h - 4; i++) {
                p[0] = 0x0F;
                p[f_w - 1] = 0xF0;
                p += SCREEN_BYTE_WIDTH;
            }

            sp_put(go_x, go_y, go_width, go_height, (uint8_t *)game_over, 0); // Вывод написи Game Over

            // Маска вместо битового поля MAG_KEY: поле gcc извлекает шестью сдвигами
            volatile uint16_t *ext_dev = (volatile uint16_t *)REG_EXT_DEV;
            volatile uint16_t *joy = (volatile uint16_t *)REG_PAR_INTERF;
            constexpr uint16_t key_up = 1 << EXT_DEV_MAG_KEY; // 1 - клавиша отпущена

            // Сначала дождаться отпускания: в момент гибели игрок обычно держит
            // стрелку, и Game Over иначе снимался бы сразу же
            while (!(*ext_dev & key_up) || (*joy & JOY_BUTTONS));

            // Ожидание нажатия клавиши или кнопки джойстика
            while ((*ext_dev & key_up) && !(*joy & JOY_BUTTONS));
            (void)*(volatile uint8_t *)REG_KEY_DATA; // Очистка буфера клавиатуры

            // Дождаться отпускания кнопки, иначе удерживаемый "огонь" сразу выстрелит в новой игре
            while (*joy & JOY_BUTTONS);

            init_game(); // Установить игру в начальное состояние
        }
    }
}

extern void start();

/**
 * @brief Основная программа
 */
void main()
{
    typedef void (*vector)();
    *((volatile vector *)VEC_STOP) = start; // Установить вектор клавиши "СТОП" на _start

    set_PSW(1 << PSW_I); // Замаскировать прерывания IRQ
    ((union KEY_STATE *)REG_KEY_STATE)->bits.INT_MASK = 1; // Отключить прерывание от клавиатуры

    (void)*(volatile uint8_t *)REG_KEY_DATA; // Очистка буфера клавиатуры

    volatile uint16_t *ptr = (uint16_t *)MEM_VIDEO;
    for (uint16_t i = 0; i < SCREEN_WORD_WIDTH * SCREEN_PIX_HEIGHT; ++i) *(ptr + i) = 0;

    volatile uint16_t *t_limit = (volatile uint16_t *)REG_TVE_LIMIT;
    volatile union TVE_CSR *tve_csr = (volatile union TVE_CSR *)REG_TVE_CSR;

    constexpr uint16_t FPS = 11; // Частота обновления кадров
    *t_limit = 3000000 / 128 / 4 / FPS;

    init_game(); // Начальная инициализация игры (init_level стартует музыку с начала)

    for (;;) // Основной бесконечный цикл игры
    {
        // Настроить таймер на использование мониторинга события, включить счётчик
        // и включить делитель на 4, а так же, сбросить флаг события таймера
        tve_csr->reg = (1 << TVE_CSR_MON) | (1 << TVE_CSR_RUN) | (1 << TVE_CSR_D4);

        plot_count = 0;

        // Диггер обрабатывается первым - чтение клавиатуры, движение, выстрел.
        // Это снимает кадр задержки между нажатием и реакцией: остальные системы
        // в этом же кадре видят новую позицию/направление/mis.fire.
        process_man();

        // На высоких уровнях сложности дать Диггеру шанс на дополнительный шаг
        // в этом же кадре (в оригинале этого нет): (d-1)/32, от 0 до 28%.
        // С 4-го уровня это чаще лишнего хода Ноббина (bugs.boost).
        if ((rand() & 0x1F) < (game.difficulty - 1)) process_man();

        // Логические координаты Диггера ПОСЛЕ хода - для process_bags
        const uint8_t man_x_log = graph_to_x_log(man.x_graph);
        const uint8_t man_y_log = graph_to_y_log(man.y_graph);

        process_bugs();
        process_bags(man_x_log, man_y_log);
        process_missile();
        process_bonus();

        // Перегруженный кадр задерживает врагов, кроме первого (delay_bugs в оригинале)
        if (plot_count > 9)
        {
            uint16_t n = plot_count - 9; // Сколько врагов задержать, начиная со второго
            if (n > MAX_BUGS - 1) n = MAX_BUGS - 1;
            struct bug_info *bug = &bugs_state[1];
            do (bug++)->wait++; while (--n);
        }

        if (snd_effects) sound_effect();
        process_game_state();

#ifdef MINIMAP
        draw_coin_minimap(); // Нарисовать мини-карту монеток (камешков)
        draw_bg_minimap();   // Нарисовать мини-карту ячеек фона
#endif

#ifdef DEBUG
        // Распечатать оставшееся свободное время
        print_dec(*((volatile uint16_t *)REG_TVE_COUNT), 0, MAX_Y_POS + 2 * POS_Y_STEP);
#endif
        if (music_on) bg_music_play(); // Воспроизвести фоновую музыку в свободном времени кадра

        // Добрать оставшееся время до конца кадра.
        while ((tve_csr->reg & (1 << TVE_CSR_FL)) == 0);
    }
}
