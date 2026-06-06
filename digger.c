#include "memory.h"
#include "sprites.h"
#include "sound.h"
#include "tools.h"
#include "emt.h"
#include "digger_sprites.h"
#include "digger_short_font.h"
#include "digger_levels.h"
#include "digger_music.h"

// #define DEBUG // Режим отладки включен
// #define MINIMAP // Отладочные карты уровня

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

constexpr uint8_t MAX_BAGS = 7; // Максимальное количество мешков с деньгами на уровне
constexpr uint8_t MAX_BUGS = 5; // Максимальное количество врагов на уровне
constexpr uint8_t MAX_LIVES = 4; // Максимальное количество жизней

constexpr uint8_t MAN_START_X = 7; // Начльное положение Диггера по оси X (вклетках)
constexpr uint8_t MAN_START_Y = 9; // Начльное положение Диггера по оси Y (вклетках)

constexpr uint8_t LOOSE_WAIT = 15; // Время с момента начала покачивания до момента падения мешка

constexpr uint32_t BONUS_LIFE_SCORE = 20000; // Количество очков для дополнительной жизни

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
    BONUS_END      /**< Режим борус закончился */
};

#pragma pack(push, 1)
/**
 * @brief Состояние мешка с деньгами
 *
 * Размер дополнен до 8 байт: при индексации &bags_state[i] компилятор использует
 * сдвиг (<<3) вместо вызова __mulhi3 для умножения на 5.
 */
struct bag_info
{
    enum bag_state state; ///< Флаг активности мешка
    enum direction dir;   ///< Направление движения мешка
    uint8_t x_graph;      ///< Положение по оси X в графических координатах
    uint8_t y_graph;      ///< Положение по оси Y в графических координатах
    uint8_t count;        ///< Счётчик
    uint8_t _pad[3];      ///< Выравнивание до 8 байт (см. комментарий выше)
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
    enum direction dir;        ///< Направление движения врага
    uint8_t count;             ///< Счётчик
    uint8_t wait;              ///< Счётчик задержки врага (при толкании мешков, изменении направления)
    uint8_t image_phase;       ///< Фаза анимации при выводе спрайта
    int8_t image_phase_inc;    ///< Направление изменения фазы анимации при выводе спрайта (+1 или -1)
    uint8_t x_graph;           ///< Положение по оси X в графических координатах
    uint8_t y_graph;           ///< Положение по оси Y в графических координатах
    uint8_t _pad[7];           ///< Выравнивание до 16 байт
};
#pragma pack(pop)

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
    uint16_t image_phase;      /// Фаза отображения спрайта Диггера
    uint16_t image_phase_inc;  /// Инкремент(декремент) фазы отображения спрайта Диггера
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
    uint8_t created;       /// Общее количество сщзданных врагов
} bugs;

// Переменные отвечающие за бонус-режим
struct {
    enum bonus_state state; /// Состояние режима бонус
    uint16_t time;          /// Время активности бонус-режима
    uint8_t  flash;         /// Время мерцания при включении/выключении Бонус-режима
    uint8_t  count;         /// Множитель очков в Бонус-режиме (умножается на два за каждого пойманного врага)
    uint32_t life_score;    /// Количество очков для дополнительное жизни
} bonus;

// Переменные отвечающие за выстрел
struct {
    uint16_t x_graph;     /// Положение выстрела по оси X в графических координатах
    uint16_t y_graph;     /// Положение выстрела по оси Y в графических координатах
    uint8_t  image_phase; /// Фаза анимации при выводе спрайта снаряда
    uint8_t  fire;        /// Флаг выстрела
    uint8_t  flying;      /// Флаг означающий, что снаряд летит
    uint8_t  wait;        /// Задержка готовности выстрела
    uint8_t  explode;     /// Счётчик взрывающегося снаряда
    enum direction dir;   /// Направление полёта выстрела
} mis;

// Переменные отвечающие за состояние игры
struct {
    uint16_t difficulty; /// Уровень сложности
    uint16_t level_no;   /// Текущий номер уровня
    int16_t  lives;      /// Текущее количество жизней
    uint32_t score;      /// Количество очков
} game;

uint8_t broke_max; // Время через которое исчезнет разбившийся мешок

// Переменные отвечающие за вывод звуков.
uint16_t snd_effects = 1;    /// Флаг, показывающий, что звуковые эффекты включены
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

#if defined(MINIMAP)
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
 * @brief Вывод 16-битного десятичного числа
 *
 * @param number - число для вывода
 * @param x_graph - координата X по которой будет осуществлён вывод числа
 * @param y_graph - координата Y по которой будет осуществлён вывод числа
 */
static void print_dec(uint16_t number, uint16_t x_graph, uint16_t y_graph)
{
    constexpr char zero = '0';
    constexpr uint16_t row_w = sizeof(digit_rows[0]); // 3 байта на строку
    constexpr uint16_t row_n = sizeof(digit_indices[0]) / sizeof(digit_indices[0][0]); // 12 строк на цифру
    char buf[5] = { zero, zero, zero, zero, zero }; // Буфер для 5 десятичных знаков

    char *ptr = &buf[sizeof(buf)];
    uint_to_str(number, &ptr);

    uint8_t digit_buf[row_n * row_w];
    for (uint8_t i = 0; i < sizeof(buf); ++i)
    {
        const uint8_t *idx_row = digit_indices[buf[i] - zero];
        uint8_t *dst = digit_buf;
        for (uint8_t r = 0; r < row_n; ++r)
        {
            const uint8_t *src = digit_rows[idx_row[r]];
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
    uint16_t man_x_offset = sizeof(digit_rows[0]) * 5 + 1; // Смещение шириной в пять символов '0' плюс один байт (4 пикселя)
    constexpr uint16_t man_y_offset = SCREEN_Y_OFFSET + 2; // Смещение спрайта Диггера по оси Y
    constexpr uint16_t one_pos_width = sizeof(image_digger_right[1][0]) + 1; // Ширина спрайта Диггера плюс один байт
    constexpr uint16_t height = sizeof(image_digger_right[1]) / sizeof(image_digger_right[1][0]); // Высота спрайта Диггера
    int16_t width = MAX_LIVES * one_pos_width; //  Общий размер места занимаемый спрайтами Диггера
    uint8_t *sprite = (uint8_t *)image_digger_right[2];

    for (uint16_t l = 1; width > 0; man_x_offset += one_pos_width, width -= one_pos_width)
    {
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
    game.score += score_add;
    print_dec(game.score, 0, SCREEN_Y_OFFSET + MOVE_Y_STEP);

     // Если количество очков досигло бонусного для получения жизни
    if (game.score >= bonus.life_score)
    {
        bonus.life_score += BONUS_LIFE_SCORE; // Количество очков до следующего бонуса в виде жизни

        // Выдать жизнь только если максимум ещё не достигнут
        if (game.lives < MAX_LIVES)
        {
            game.lives++; // Увеличичить количество жизней на единицу
            print_lives(); // Вывесли количество жизней
            snd.life = 24; // Издать звук получения жизни
        }
    }
}

/**
 * @brief Добавление очков за убитого врага
 */
static void add_score_250()
{
    add_score(250); // 250 очков за убитого врага
}

/**
 * @brief Проверка соприкосновения двух 4x15-спрайтов по их левым-верхним углам.
 */
static int check_collision_4_15(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    return ((uint16_t)((int)x2 - (int)x1 + 3) < 7u)
        && ((uint16_t)((int)y2 - (int)y1 + 14) < 29u);
}

/**
 * @brief Преобразование графической координаты X в логическую (номер клетки).
 */
static uint8_t graph_to_x_log(uint16_t x_graph)
{
    return (x_graph - FIELD_X_OFFSET) / POS_X_STEP;
}

static uint8_t graph_to_y_log(uint16_t y_graph)
{
    return (y_graph - FIELD_Y_OFFSET) / POS_Y_STEP;
}

/**
 * @brief Получение ячейки уровня по заданным координатам.
 */
static inline enum level_symbols getLevelSymbol(uint8_t y_log, uint8_t x_log)
{
    static const uint8_t word_no_tbl[W_MAX] = { 0,0,0,0,0, 1,1,1,1,1, 2,2,2,2,2 };
    static const uint8_t shift_tbl[W_MAX]   = { 0,3,6,9,12, 0,3,6,9,12, 0,3,6,9,12 };

    return (level[game.level_no][y_log][word_no_tbl[x_log]] >> shift_tbl[x_log]) & 7;
}

static void bonus_indicator(uint16_t color);

/**
 * @brief Инициализация переменных состояния перед старом уровня
 */
static void init_level_state()
{
    // Отключить бонус-режим
    bonus.state = BONUS_OFF;

    // Стереть вишенку
    erase_4_15(FIELD_X_OFFSET + (W_MAX - 1) * POS_X_STEP, FIELD_Y_OFFSET);

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

        sp_4_15_mask(bag->x_graph, bag->y_graph, nullptr, outline_bag[0]); // Стереть мешок
        bag->state = BAG_INACTIVE;
    }

    // print_dec(game.difficulty, 0, MAX_Y_POS + 2 * POS_Y_STEP);

    if (game.difficulty > 6) bugs.max = 5;      // На уровне сложности 7 и выше максимально 5 врагов одновременно
    else if (game.difficulty > 0) bugs.max = 4; // На уровне сложности со 1 до 6 (включительно) до 4 врагов одновременно
    else bugs.max = 3;                      // На первом уровне максимально три варага одновременно

    // Переменные относщиеся к созданию и управлению врагами
    bugs.total = game.difficulty + 6;         // Общее количество врагов на уровне - шесть плюс уровень сложности
    bugs.delay = 45 - (game.difficulty << 1); // Задержка появления врагов (с ростом сложности убывает)
    bugs.delay_counter = bugs.delay;     // Инициализация счётчика задержки врага исходным значением
    bugs.active = 0;                     // Количество активных врагов
    bugs.created = 0;                    // Общее количество сщзданных врагов

    broke_max = 140 - game.difficulty * 10; // Время через которое исчезнет разбившийся мешок (с ростом сложности убывает)

    // Инициализация переменных Диггера
    man.dir = DIR_RIGHT;
    man.prev_dir = DIR_RIGHT;
    man.x_graph = FIELD_X_OFFSET + MAN_START_X * POS_X_STEP; // Исходная координата Диггера на экране по оси X
    man.y_graph = FIELD_Y_OFFSET + MAN_START_Y * POS_Y_STEP; // Исходная координата Диггера на экране по оси Y
    man.image_phase = 0;        // Фаза анимации Диггера
    man.image_phase_inc = 1;    // Направление ихменения фазы анимации Диггера
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
    static const struct
    {
        int16_t x;
        int16_t y;
        uint16_t x_size;
        uint16_t y_size;
        uint8_t *sprite;
    } gnaw_mtx[4] = {
        { -2, -1, sizeof(outline_blank_left[0]),  sizeof(outline_blank_left)  / sizeof(outline_blank_left[0]),  (uint8_t*)outline_blank_left  },
        {  4, -1, sizeof(outline_blank_right[0]), sizeof(outline_blank_right) / sizeof(outline_blank_right[0]), (uint8_t*)outline_blank_right },
        { -1, -7, sizeof(outline_blank_up[0]),    sizeof(outline_blank_up)    / sizeof(outline_blank_up[0]),    (uint8_t*)outline_blank_up    },
        { -1, 15, sizeof(outline_blank_down[0]),  sizeof(outline_blank_down)  / sizeof(outline_blank_down[0]),  (uint8_t*)outline_blank_down  }
    };

    sp_put(x_graph + gnaw_mtx[dir].x, y_graph + gnaw_mtx[dir].y, gnaw_mtx[dir].x_size, gnaw_mtx[dir].y_size, nullptr, gnaw_mtx[dir].sprite);
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

    const uint8_t *back_image = (uint8_t *)image_background[game.level_no]; // Указатель на образец фона для текущего уровня

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
    for (uint16_t y_log = 0; y_log < H_MAX; ++y_log)
    {
        coins[y_log] = 0; // Сброситть все биты монеток для данной строки

        for (uint16_t x_log = 0; x_log < W_MAX; ++x_log)
        {
            uint8_t *bg = &background[y_log][x_log]; // Структура с информацией о клетке фона
            *bg = 0; // Сбросить все биты состояния фона (вся клетка фона цела)

            enum level_symbols ls = getLevelSymbol(y_log, x_log);

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
                sp_4_15_mask(bag->x_graph, bag->y_graph, image_bag[0], outline_bag[0]);
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
                for (uint16_t i = 15; i > 0; i -= 3)
                {
                    gnaw(DIR_DOWN,x_graph, y_graph - i);
                }
                gnaw(DIR_UP, x_graph, y_graph + 3);
            }

            x_graph += POS_X_STEP;
        }

        x_graph = FIELD_X_OFFSET;
        y_graph += POS_Y_STEP;
    }

    init_level_state();  // Инициализировать состояние уровня
}

/**
 * @brief Определяет по состоянию байта клетки, что клетка  проедена
 *
 * @param byte - байт с состоянием клетки
 *
 * @return - 1 - клетка проедена, 0 - клетка не проедена
 */
static uint16_t full_bite(uint8_t byte)
{
    return byte & (byte - 1);

    // int bits_count;
    // for (bits_count = 0; byte; bits_count++)
    // {
    //     byte &= byte - 1;
    // }
    //
    // return bits_count > 1;
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
static uint8_t check_path(enum direction dir, uint8_t x_graph, uint8_t y_graph)
{
    uint8_t x_log = graph_to_x_log(x_graph);
    uint8_t y_log = graph_to_y_log(y_graph);
    const uint8_t current_cell = background[y_log][x_log]; // Состояние текущей клетки

    static const struct
    {
        int8_t  x;
        int8_t  y;
        uint8_t mask;
        uint8_t cur_mask;
    } dir_matrix[4] = {
        { -1,  0, 0x08, 0x01 }, // Влево
        {  1,  0, 0x01, 0x08 }, // Вправо
        {  0, -1, 0x80, 0x10 }, // Вверх
        {  0,  1, 0x10, 0x80 }  // Вниз
    } ;

    x_log += dir_matrix[dir].x;
    y_log += dir_matrix[dir].y;

    if ((x_log >= W_MAX) || (y_log >= H_MAX)) return 0;

    const uint8_t neighbor_cell = background[y_log][x_log]; // Состояние соседней клетки
    if (!full_bite(neighbor_cell)) return 0;
    if (neighbor_cell & dir_matrix[dir].mask) return 1;
    return current_cell & dir_matrix[dir].cur_mask;
}

/**
 * @brief Очистить биты состоянияфона по заданным координатам в указанном направлении
 *
 * @param x_graph - графическая координата по оси X
 * @param y_graph - графическая координата по оси Y
 * @param dir - направлкние движения
 */
static void set_background_bits(uint16_t x_graph, uint16_t y_graph, enum direction dir)
{
    const uint16_t abs_x_pos = x_graph - FIELD_X_OFFSET;
    const uint16_t abs_y_pos = y_graph - FIELD_Y_OFFSET;
    uint16_t x_log = abs_x_pos / POS_X_STEP;
    uint16_t y_log = abs_y_pos / POS_Y_STEP;
    int16_t x_rem = abs_x_pos % POS_X_STEP;
    int16_t y_rem = (abs_y_pos % POS_Y_STEP) >> 2;

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

    switch (dir)
    {
        case DIR_LEFT:
        case DIR_RIGHT:
        {
            *cell |= 1 << x_rem; // Установить соответсвующий бит матрицы фона
            break;
        }

        case DIR_UP:
        case DIR_DOWN:
        {
            *cell |= 1 << (y_rem + 4); // Установить соответсвующий бит матрицы фона
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

    uint8_t bag_x_graph = bag->x_graph;
    uint8_t bag_y_graph = bag->y_graph;

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
        // Стирание мешка по старым координатам
        sp_4_15_mask(bag->x_graph, bag->y_graph, nullptr, outline_bag[0]);

        // Отрисовка спрайта передвигаемого мешка
        sp_4_15_mask(bag_x_graph, bag_y_graph, image_bag[0], outline_bag[0]);

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

    const uint8_t bug_x_graph = bug->x_graph;
    const uint8_t bug_y_graph = bug->y_graph;
    const uint8_t bug_abs_x_pos = bug_x_graph - FIELD_X_OFFSET;
    const uint8_t bug_abs_y_pos = bug_y_graph - FIELD_Y_OFFSET;
    const uint8_t bug_x_rem = bug_abs_x_pos % POS_X_STEP;
    const uint8_t bug_y_rem = (bug_abs_y_pos % POS_Y_STEP) >> 2;

    // Проверка возможности изменения направления движения при нахождении на ровной границе клетки
    if (!bug_x_rem && !bug_y_rem)
    {
        // Если Хоббин застрял на время более заданного, то превратить его в Ноббина
        if ((bug->type == BUG_HOBBIN) && (bug->count > (32 + game.difficulty * 2)))
        {
            bug->count = 0;         // Очистить время застревания
            bug->type = BUG_NOBBIN; // Превратить врага в Ноббина
        }

        // Поиск порядка наилучших направлений движения
        dir_1 = (man.x_graph < bug_x_graph) ? DIR_LEFT : DIR_RIGHT;
        dir_2 = (man.y_graph < bug_y_graph) ? DIR_UP   : DIR_DOWN;

        // Если расстояние по горизонтали превышает расстояние по вертикали, то отдать приоритет оси X
        if (abs16(man.y_graph - bug_y_graph) > abs16(man.x_graph - bug_x_graph))
        {
            // Если расстояние по вертикали превышает расстояние по горизонтали, то отдать приоритет оси Y
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

        if (dir == dir_1) // Если движение назад наболее приоритетно
        {
            // Сдвинуть все направления, вместо последнего использовать движение назад
            dir_1 = dir_2;
            dir_2 = dir_3;
            dir_3 = dir_4;
            dir_4 = dir;
        }

        if (dir == dir_2) // Если движение назад более приоритетное
        {
            // Сдвинуть все направления, кроме наиболее приоритетного,
            // вместо последнего использовать движение назад
            dir_2 = dir_3;
            dir_3 = dir_4;
            dir_4 = dir;
        }

        if (dir == dir_3) // Если движение назад менее приоритетное
        {
            // Вместо менее приоритетного направления использовать наименее приоритетное,
            // вместо последнего использовать движение назад
            dir_3 = dir_4;
            dir_4 = dir;
        }

        // В уровнях сложности до шестого использовать элемент случайности в выборе направления
        if ((game.difficulty < 5) && ((rand() & 0xF) > (game.difficulty + 10)))
        {
            // В одном из (5 + game.difficulty) случаев поменять наиболее
            // приоритетное направление с менее приоритетным
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

        // Стерерь кусочек фона на экране в соответствии с направлением движения и текущим положением
        gnaw(bug->dir, bug_x_graph, bug_y_graph);

        remove_coin(graph_to_x_log(bug_x_graph), graph_to_y_log(bug_y_graph));
    }

    if (man.state == CREATURE_ALIVE) // Если Диггер жив
    {
        // Выждать время задержки перед запуском нового врага
        if (bug->state == CREATURE_STARTING)
        {
            if (bug->count > 0) bug->count--;
            else bug->state = CREATURE_ALIVE; // Если счётчик закончился, что оживить врага
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
        // Проверить соприкосновение врага с мешками
        for (uint8_t i = 0; i < MAX_BAGS; ++i)
        {
            struct bag_info *bag = &bags_state[i]; // Структура с информацией о мешках

            if (bag->state == BAG_INACTIVE) continue; // Пропустить неактивные мешки

            if (check_collision_4_15(bag->x_graph, bag->y_graph, bug_x_graph, bug_y_graph))
            {
                uint16_t remove_bag = 0;
                if (bug->type == BUG_HOBBIN)
                {
                    // Если Хоббин коснулся мешка
                    remove_bag = 1; // Удалить съеденный Хоббином мешок
                }
                else
                {
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
                                bug->wait++; // Задержать врага перед мешком

                                if ((dir == DIR_UP) || (dir == DIR_DOWN))
                                {
                                    bug->dir ^= 1; // Инвертировать направление движения врага
                                }

                                break;
                            }

                            break;
                        }

                        case BAG_BROKEN:  // Если мешок разбит
                        {
                            remove_bag = 1; // Удалить съеденное Ноббином золото
                            bug->wait++; // Задержать врага съедающего золото
                            break;
                        }
                    }
                }

                if (remove_bag)
                {
                    bag->state = BAG_INACTIVE; // Деактивировать мешок

                    // Стереть съеденный мешок или золото
                    sp_4_15_mask(bag->x_graph, bag->y_graph, nullptr, outline_bag_fall[0]);

                    bug->wait++; // Задержать врага перед мешком
                }
            }
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

    // Увеличить/уменьшить фазу на единицу
    bug->image_phase += bug->image_phase_inc;

    // Переключить направление изменения фазы, если фаза дошла до предельного значения
    if (!bug->image_phase || bug->image_phase >= 2) bug->image_phase_inc = -bug->image_phase_inc;

    // Отрисовка спрайта врага
    if (bug->type == BUG_NOBBIN)
    {
        sp_4_15_put(bug->x_graph, bug->y_graph, (uint8_t *)image_nobbin[bug->image_phase]);
    }
    else if (bug->dir == DIR_RIGHT)
    {
        sp_4_15_put(bug->x_graph, bug->y_graph, (uint8_t *)image_hobbin_right[bug->image_phase]);
    }
    else
    {
        sp_4_15_h_mirror_put(bug->x_graph, bug->y_graph, (uint8_t *)image_hobbin_right[bug->image_phase]);
    }
}

/**
 * @brief Остановка мешка
 *
 * @param bag - указатель на структуру с информацией о мешке
 */
static void stop_bag(struct bag_info *bag)
{
    // Если мешок пролетел больше одного этажа, то он будет разбит
    bag->state = (bag->count > 1) ? BAG_BREAKS : BAG_STATIONARY;
    bag->dir = DIR_STOP; // Остановить мешок
    bag->count = 0;

    // TODO: Унчтожить все мешки под тем, который остановился
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
 * @brief Подпрограмма отрисовки Диггера
 */
static void draw_man()
{
    uint8_t cab = !mis.flying && !mis.wait; // Флаг наличия "башенки"

    man.image_phase += man.image_phase_inc; // Переключить фазу изображения

    // При необходимости, сменить направление изменения фазы спрайта
    if (!man.image_phase || man.image_phase >= 2) man.image_phase_inc = -man.image_phase_inc;

    uint16_t image_phase = man.image_phase + ((cab) ? 0 : 3);
    const uint8_t *image = (man.dir < DIR_UP) ? (uint8_t *)image_digger_right[image_phase] : (uint8_t *)image_digger_up[image_phase];

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
        sound(period, 30);
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
            sound(explode_snd_period, 30);
        }
    }

    if (snd.loose) // Звук раскачивающегося мешка
    {
        static const uint16_t loose_periods[] = { 2500 / N, 3000 / N, 2500 / N, 2000 / N };
        static const uint16_t loose_durances[] = { 24, 20, 24, 30 };

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
            sound(period, 25);
            snd.money--;
        }
    }

    if (snd.chase) // Звук включения бонус-режима
    {
        uint16_t durance = 75;
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
                sound(snd.bug_period_held / 16, 25);
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
        sound(14 + snd.life, 40);
        snd.life -= 2;
    }
}

/**
 * @brief Инициализация игры
 */
static void init_game()
{
    game.difficulty = 0; // Начальный уровень сложности
    game.level_no = 0;   // Начальный уровень
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
    else
    {
        bugs.delay_counter = bugs.delay; // Перезарядить счётчик времени до появления врага

        if (man.state == CREATURE_ALIVE) // Если Диггер жив
        {
            if ((bugs.created < bugs.total) && (bugs.active < bugs.max)) // Если врагов на экране одновременно меньше максимального количества
            {
                if (bonus.state != BONUS_ON) // Если не включен бонус-режим, запустить нового врага
                {
                    // Координаты рожденияя врагов - в правом верхнем углу
                    constexpr uint8_t bug_start_x = FIELD_X_OFFSET + (W_MAX - 1) * POS_X_STEP;
                    constexpr uint8_t bug_start_y = FIELD_Y_OFFSET + 0 * POS_Y_STEP;

                    for (uint16_t i = 0; i < bugs.max; ++i)
                    {
                        struct bug_info *bug = &bugs_state[i];

                        if (bug->state != CREATURE_INACTIVE) continue; // Пропустить активных врагов

                        // Начальное состояние врага
                        bug->state = CREATURE_STARTING; // Враг стартует
                        bug->count = 6;                 // Время до запуска врага
                        bug->wait = 0;                  // Враг не задержан
                        bug->image_phase = 0;           // Начальная фаза орисовки спрайта
                        bug->image_phase_inc = 1;       // Начальное направление изменения фазы
                        bug->x_graph = bug_start_x;     // Начальная графическая координата по оси X
                        bug->y_graph = bug_start_y;     // Начальная графическая координата по оси Y
                        bug->type = BUG_NOBBIN;         // Враги рождаются в виде Ноббинов
                        bug->dir = DIR_STOP;            // Начальное направление движения

                        bugs.active++;  // Увеличить счётчик активных врагов
                        bugs.created++; // Увеличить общее количество созданных врагов

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
    }

    // Обработка врагов (Ноббинов и Хоббинов)
    for (uint16_t i = 0; i < bugs.max; ++i)
    {
        struct bug_info *bug = &bugs_state[i]; // Структура с информацией о враге

        if (bug->state == CREATURE_INACTIVE) continue; // Пропустить, если враг не активен

        switch (bug->state)
        {
            case CREATURE_ALIVE: // Перемещение живого врага
            {
                // Если враг в режиме ожидания
                if (bug->wait)
                {
                    bug->wait--; // Уменьшить счётчик в режиме ожидания
                    break;
                }

                if (bug->type == BUG_NOBBIN) // Если это Ноббин
                {
                    for (uint16_t t = 0; t < bugs.max; ++t)
                    {
                        if (t == i) continue; // Пропустить самого себя

                        struct bug_info *another_bug = &bugs_state[t];
                        if (another_bug->state != CREATURE_ALIVE) continue; // Пропустить неживых врагов

                        // Если враг соприкоснулся с другим врагом
                        if (check_collision_4_15(bug->x_graph, bug->y_graph, another_bug->x_graph, another_bug->y_graph))
                        {
                            bug->count++;  // Увеличить счётчик застревания

                            if (bug->dir == another_bug->dir) // Если враги движутся в одном направлении
                            {
                                bug->wait++;   // Увеличить счётчик ожидания
                                bug->dir ^= 1; // Инвертировать направление движения
                            }
                        }
                    }

                    //  Если Ноббин застрял или соприкоснулся с другим на определённое (зависящее от уровня сложности) время
                    if (bug->count > (20 - game.difficulty))
                    {
                        bug->count = 0;         // Сбросить счётчик застревания
                        bug->type = BUG_HOBBIN; // Переключить тип врага на Хоббина
                    }
                }

                // Если выпало случайное число с вероятностью зависящей от уровня сложности
                if ((rand() & 0xF) < game.difficulty) move_bug(bug); // Переместить врага ещё раз для увеличения скорости

                // Здесь специально нету break для проваливания в следующую секцию
            }

            case CREATURE_STARTING: // Враг ждёт старта
            {
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
                bug->state = CREATURE_INACTIVE;         // Декативировать убитого врага
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

        uint8_t bag_x_graph = bag->x_graph;
        uint8_t bag_y_graph = bag->y_graph;
        uint8_t bag_abs_x_pos = bag_x_graph - FIELD_X_OFFSET;
        uint8_t bag_abs_y_pos = bag_y_graph - FIELD_Y_OFFSET;
        uint8_t bag_x_log = bag_abs_x_pos / POS_X_STEP;
        uint8_t bag_y_log = bag_abs_y_pos / POS_Y_STEP;
        uint8_t bag_x_rem = bag_abs_x_pos % POS_X_STEP;
        uint8_t bag_y_rem = (bag_abs_y_pos % POS_Y_STEP) >> 2;

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
                                // Если Диггер двигался вверх и он находится под мешком, то пока не начинать раскачивать мешок
                                if (!((man_x_log == bag_x_log) && (man_y_log == bag_y_log + 1) && (man.new_dir == DIR_UP)))
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
                                snd.fall = 1;             // Включить звук падения мешка

                                break;
                            }
                        }
                    }

                    bag->dir = DIR_STOP; // Остановить мешок
                }

                if (bag->dir != DIR_STOP) // Если мешок не остановлен
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
                    }
                }
                else
                {
                    // Если счётчик времени до падения мешка закончился
                    bag->state = BAG_FALLING; // Начать падение мешка
                    bag->dir = DIR_DOWN;      // Направление движения мешка - вниз
                    bag->count = 0;           // Сбросить счётчик этажей
                    snd.fall = 1;             // Включить звук падения мешка
                }

                break;
            }

            case BAG_FALLING: // Мешок падает
            {
                bags_fall = 1; // Найден падающий мешок

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
                bag_abs_y_pos = bag_y_graph - FIELD_Y_OFFSET;
                bag_y_log = bag_abs_y_pos / POS_Y_STEP;
                bag_y_rem = (bag_abs_y_pos % POS_Y_STEP) >> 2;

                if (bag_y_rem == 0) // Если мешок находится в центре клетки по-вертикали, значит он пролетел один этаж
                {
                    bag->count++; // Увеличить количество этажей, которые пролетел мешок

                    // Остановить мешок, если клетка под ним не повреждена или он долетел до последнего этажа
                    if ((bag_y_log == H_MAX - 1) || !background[bag_y_log + 1][bag_x_log]) stop_bag(bag);
                }

                remove_coin(bag_x_log, bag_y_log); // Удалить монету в клетке куда попал мешок

                // Нарисовать падающий мешок
                sp_4_15_put(bag_x_graph, bag_y_graph, (uint8_t *)image_bag_fall);
                // sp_put(bag_x_graph, bag_y_graph, sizeof(image_bag_fall[0]), sizeof(image_bag_fall) / sizeof(image_bag_fall[0]),
                //         (uint8_t *)image_bag_fall, (uint8_t *)outline_bag_fall);

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
                    if (bug->state != CREATURE_ALIVE) continue; // Пропустить неживых врагов

                    uint8_t bug_x_graph = bug->x_graph;

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
                uint16_t *v_scroll = (uint16_t *)REG_V_SCROLL;

                // Анимация рабивающегося мешка (три фазы, пропуская один такт счётчика)
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
                        // Нарисовать анимацию рассыпающегося золота
                        sp_4_15_put(bag->x_graph, bag->y_graph, (uint8_t *)image_bag_broke[(bag->count - 1) >> 1]);
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
    constexpr uint16_t missile_phases_no = sizeof(image_missile) / sizeof(image_missile[0]);

    // Размеры и количество фаз анимации взрыва
    constexpr uint16_t explode_x_size = sizeof(image_explode[0][0]);
    constexpr uint16_t explode_y_size = sizeof(image_explode[0]) / explode_x_size;
    constexpr uint16_t explode_phases_no = sizeof(image_explode) / sizeof(image_explode[0]);

    if (mis.explode)
    {
        // Обработка взрывающегося выстрела
        if (mis.image_phase < explode_phases_no)
        {
            // Вывести изображение взрыва
            sp_put(mis.x_graph, mis.y_graph, explode_x_size, explode_y_size, (uint8_t *)image_explode[mis.image_phase++], nullptr);
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
                if (bug->state != CREATURE_ALIVE) continue; //  Пропустить неживых врагов

                // Проверить, что выстрел попал во врага
                if (check_collision_4_15(mis.x_graph, mis.y_graph, bug->x_graph, bug->y_graph))
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
                // Циклически менять фазу анимации выстрела
                if (++mis.image_phase >= missile_phases_no) mis.image_phase = 0;

                // Вывести новое изображение выстрела
                sp_put(mis.x_graph, mis.y_graph, missile_x_size, missile_y_size, (uint8_t *)image_missile[mis.image_phase], nullptr);
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
                    mis.wait = 85 + game.difficulty * 4; // Начальное значение счётчика появления "башенки"
                    mis.image_phase = 0;
                    mis.flying = 1;
                    mis.dir = man.dir;

                    // Определить начальное положение выстрела в зависимости от
                    // координат Диггера и его направления движения.
                    // Смещения нерегулярные (специфичные точки рождения снаряда
                    // относительно спрайта Диггера), общую таблицу dir_d* не используем.
                    static const int8_t fire_dx[4] = { -MOVE_X_STEP, 4, 1, 1 };
                    static const int8_t fire_dy[4] = { MOVE_Y_STEP, MOVE_Y_STEP, -MOVE_Y_STEP, 15 + MOVE_Y_STEP };

                    mis.x_graph = man.x_graph + fire_dx[mis.dir];
                    mis.y_graph = man.y_graph + fire_dy[mis.dir];

                    // Вывести начальное положение спрайта выстрела
                    sp_put(mis.x_graph, mis.y_graph, missile_x_size, missile_y_size, (uint8_t *)image_missile[mis.image_phase], nullptr);

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
static void process_man(const uint8_t man_x_rem, const uint8_t man_y_rem)
{
    // Обработка перемещения Диггера
    if (man.state == CREATURE_ALIVE) // Если Диггер жив
    {
        if (man.wait) man.wait--; // Если Диггер в режиме задержки (при толкании мешков)
        else
        {
            man.new_dir = DIR_STOP;

            // Обработка управления с клавиатуры и джойстика
            volatile uint16_t port_state = *((uint16_t *)REG_PAR_INTERF); // Состояние регистра параллельного порта
            // print_dec(port_state, 0, MAX_Y_POS + 2 * POS_Y_STEP);
            volatile uint8_t key_pressed = !(((union EXT_DEV *)REG_EXT_DEV)->bits.MAG_KEY);
            volatile uint8_t new_code = (*(uint8_t *)REG_KEY_STATE) & (1 << KEY_STATE_STATE);
            volatile uint8_t code = *((uint8_t *)REG_KEY_DATA); // Скан-код нажатой клавиши
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

            if (!mis.wait && (port_state & ((1 << PAR_INTERF_LEFT_BUTTON) | (1 << PAR_INTERF_RIGHT_BUTTON)))) mis.fire = 1;

            if (new_code) // Если поступил новый скан-код
            {
                switch (code)
                {
                    case 12:  // СБР - Пауза
                    {
                        while (!((*(volatile uint8_t *)REG_KEY_STATE) & (1 << KEY_STATE_STATE)));
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
#if defined(DEBUG)
                    case 'D': // Увеличение уровня сложности
                    {
                        if (++game.difficulty >= 10) game.difficulty = 0;
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
                if (check_collision_4_15(man.x_graph, man.y_graph, FIELD_X_OFFSET + (W_MAX - 1) * POS_X_STEP, FIELD_Y_OFFSET))
                {
                    bonus.state = BONUS_ON; // Включить Бонус-режим
                    bonus.count = 1; // Начальное значение множителя очков в Бонус-режиме
                    bonus.time = 230 - game.difficulty * 20; // Время действия Бонус-режима
                    bonus.flash = 19; // Время мигания индикатора включения Бонус-режима

                    add_score(1000); // 1000 очков за вишенку

                    // Стереть вишенку
                    erase_4_15(FIELD_X_OFFSET + (W_MAX - 1) * POS_X_STEP, FIELD_Y_OFFSET);
                }
                else
                {
                    // Нарисовать вишенку в правом верхнем углу игрового поля
                    sp_4_15_put(FIELD_X_OFFSET + (W_MAX - 1) * POS_X_STEP, FIELD_Y_OFFSET, (uint8_t *)image_cherry);
                }
            }

            uint8_t prev_man_x_graph = man.x_graph;
            uint8_t prev_man_y_graph = man.y_graph;

            if (man.dir != DIR_STOP)
            {
                // Переместить Диггера на один шаг в заданном направлении
                man.x_graph += dir_dx[man.dir] * MOVE_X_STEP;
                man.y_graph += dir_dy[man.dir] * MOVE_Y_STEP;
            }
            else man.dir = man.prev_dir;

            uint16_t collision_flag = 0;

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
                            collision_flag = 1;
                        }

                        break;
                    }

                    case BAG_FALLING:
                    {
                        collision_flag = 1;
                        break;
                    }

                    case BAG_BREAKS:
                    case BAG_BROKEN:
                    {
                        // Включить звук съедаемого золота
                        snd.money_period_1 = 500 / N; // Начальный период чётных звуков
                        snd.money_period_2 = 4000 / N; // Начальный период нечётных звуков
                        snd.money = 30; // Количество звуков в последовательности
                        bag->state = BAG_INACTIVE; // Сделать мешок неактивным
                        add_score(500); // 500 очков за съеденное золото
                        // Стереть золото из разбитого мешка
                        erase_4_15(bag->x_graph, bag->y_graph);
                        break;
                    }
                }
            }

            if (collision_flag)
            {
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

                if (bug->state != CREATURE_ALIVE) continue; // Пропустить дохлых врагов

                // Если Диггер не касается врага
                if (!check_collision_4_15(bug->x_graph, bug->y_graph, man.x_graph, man.y_graph)) continue;

                if (bonus.state == BONUS_ON)
                {
                    // Если включен режим Бонус
                    snd.bug = 1; // Запустить воспроизведение звука съедения врага
                    snd.bug_c1 = 0;
                    snd.bug_c2 = 4;
                    snd.bug_period = 0;
                    add_score(bonus.count * 200); // 200 * bonus.count очков за каждого съеденного врага
                    bonus.count <<= 1; // Удвоить bonus.count

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

            if (man.dead_bag->dir == DIR_STOP)
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
            bug->state = CREATURE_INACTIVE; // Декативировать врага убившего Диггера
            erase_4_15(bug->x_graph, bug->y_graph); // Стереть деактивированного врага
        }
    }

    // Траурный марш
    static const uint8_t music_dead_periods[]   = { C4, C4, C4, C4, DS4, D4, D4, C4, C4, B3, C4 };
    static const uint16_t music_dead_durations[] = { N6, NQ, NE, N6, NQ, NE, NQ, NE, NQ, NE, N12 };

    // Фазы орисовки надгробного камня
    static const uint8_t rip_frames[5][2] = {
        { 10,  4 }, // верхушка надгробия только показалась
        {  8,  6 },
        {  6,  8 },
        {  3, 11 },
        {  1, 14 }, // полный надгробный камень
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

    (void)*(volatile uint8_t *)REG_KEY_DATA;

    man.state = CREATURE_RIP;
}

static void bonus_indicator(uint16_t color)
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

                // Мигание при включении бонус-режима
                bonus_indicator((bonus.time & 1) ? (bonus.flash ? 0xFFFF : 0xAAAA) : 0x0000);

                // TODO: Включить музыку бонус-режима
            }
        }
        else
        {
            bonus.state = BONUS_END;
            snd.chase = 0; // Выключить звук включения/выключения бонус-режима
            bugs.delay_counter = 0; // Враги начинают появляться сразу же после окончания бонус-режима

            // TODO: Включить музыку Popcorn
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

        // Циклическое увеличение номера уровня
        game.level_no++;
        game.level_no &= LEVELS_NUM - 1;

        // Увеличение сложности после прохождения очередного уровня (максимальный уровень 9)
        if (game.difficulty < 10) game.difficulty++;

        init_level(); // Инициализация нового уровня
    }

    if (man.state == CREATURE_RIP)
    {
        game.lives--;                    // Уменьшить количество жизней
        print_lives();              // Вывести количество жизней

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

            while(((union EXT_DEV *)REG_EXT_DEV)->bits.MAG_KEY); // Ожидание нажатия клавиши
            (void)*(volatile uint8_t *)REG_KEY_DATA; // Очистка буфера клавиатуры

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
    // EMT_14();
    // EMT_16(0233);
    // EMT_16(0236);

    typedef void (*vector)();
    *((volatile vector *)VEC_STOP) = start; // Установить вектор клавиши "СТОП" на _start

    set_PSW(1 << PSW_I); // Замаскировать прерывания IRQ
    ((union KEY_STATE *)REG_KEY_STATE)->bits.INT_MASK = 1; // Отключить прерывание от клавиатуры

    volatile uint16_t *ptr = (uint16_t *)MEM_VIDEO;
    for (uint16_t i = 0; i < SCREEN_WORD_WIDTH * (FIELD_Y_OFFSET - MOVE_Y_STEP * 3); ++i) *(ptr + i) = 0;

    volatile uint16_t *t_limit = (volatile uint16_t *)REG_TVE_LIMIT;
    volatile union TVE_CSR *tve_csr = (volatile union TVE_CSR *)REG_TVE_CSR;

    constexpr uint16_t FPS = 11; // Частота обновления кадров
    *t_limit = 3000000 / 128 / 4 / FPS;

    init_game(); // Начальная инициализация игры

    for (;;) // Основной бесконечный цикл игры
    {
        // Настроить таймер на использование мониторинга события, включить счётчик и включить делитель на 4,
        // а так же, сбросить флаг события таймера
        tve_csr->reg = (1 << TVE_CSR_MON) | (1 << TVE_CSR_RUN) | (1 << TVE_CSR_D4);

        // Положение Диггера на момент входа в кадр: остатки нужны process_man
        // чтобы понять, на границе ли клетки и можно ли менять направление.
        const uint8_t man_x_rem = (man.x_graph - FIELD_X_OFFSET) % POS_X_STEP;
        const uint8_t man_y_rem = ((man.y_graph - FIELD_Y_OFFSET) % POS_Y_STEP) >> 2;

        // Диггер обрабатывается первым - чтение клавиатуры, движение, выстрел.
        // Это снимает кадр задержки между нажатием и реакцией: остальные системы
        // в этом же кадре видят новую позицию/направление/mis.fire.
        process_man(man_x_rem, man_y_rem);

        // Логические координаты Диггера ПОСЛЕ хода - для process_bags
        const uint8_t man_x_log = (man.x_graph - FIELD_X_OFFSET) / POS_X_STEP;
        const uint8_t man_y_log = (man.y_graph - FIELD_Y_OFFSET) / POS_Y_STEP;

        process_bugs();
        process_bags(man_x_log, man_y_log);
        process_missile();
        process_bonus();
        if (snd_effects) sound_effect();
        process_game_state();

#if defined(MINIMAP)
        draw_coin_minimap(); // Нарисовать мини-карту монеток
        draw_bg_minimap();   // Нарисовать мини-карту ячеек фона
#endif

#if defined(DEBUG)
        // Рспечатать оставшееся свободное время
        print_dec(*((volatile uint16_t *)REG_TVE_COUNT), 0, MAX_Y_POS + 2 * POS_Y_STEP);
#endif
        while ((tve_csr->reg & (1 << TVE_CSR_FL)) == 0); // Ожидать срабатывания таймера.
    }
}
