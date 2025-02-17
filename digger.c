#include "memory.h"
#include "sprites.h"
#include "sound.h"
#include "tools.h"
#include "emt.h"
#include "digger_sprites.h"
#include "digger_font.h"
#include "digger_levels.h"
#include "digger_music.h"

// #define DEBUG // Режим отладки включен

#define SCREEN_Y_OFFSET 25

#define FIELD_X_OFFSET 2  // Смещение игрового поля по оси X
#define FIELD_Y_OFFSET (SCREEN_Y_OFFSET + 32) // Смещение игрового поля по оси Y
#define POS_X_STEP 4      // Шаг клеток по оси X (в байтах)
#define POS_Y_STEP 16     // Шаг клеток по оси Y (в строках)
#define MOVE_X_STEP 1     // Шаг перемещения по оси X (в байтах)
#define MOVE_Y_STEP 4     // Шаг перемещения по оси Y (в строках)

#define MIN_X_POS FIELD_X_OFFSET // Минимальное положение по оси X
#define MIN_Y_POS FIELD_Y_OFFSET // Минимальное положение по оси Y
#define MAX_X_POS (FIELD_X_OFFSET + POS_X_STEP * (W_MAX - 1)) // Максимальное положение по оси X
#define MAX_Y_POS (FIELD_Y_OFFSET + POS_Y_STEP * (H_MAX - 1)) // Максимальное положение по оси Y

#define COIN_Y_OFFSET 3 // Смещение спрайта монетки в ячейке по оси Y

#define MAX_BAGS 7 // Максимальное количество мешков с деньгами на уровне
#define MAX_BUGS 5 // Максимальное количество врагов на уровне

#define MAN_START_X 7 // Начльное положение Диггера по оси X (вклетках)
#define MAN_START_Y 9 // Начльное положение Диггера по оси Y (вклетках)

#define LOOSE_WAIT 15 // Время с момента начала покачивания до момента падения мешка

#define BONUS_LIFE_SCORE 20000 // Количество очков для дополнительной жизни
#define MAX_LIVES 4 // Максимальное количество жизней

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
 */
struct bag_info
{
    enum bag_state state; ///< Флаг активности мешка
    enum direction dir;   ///< Направление движения мешка
    uint8_t x_graph;      ///< Положение по оси X в графических координатах
    uint8_t y_graph;      ///< Положение по оси Y в графических координатах
    uint8_t count;        ///< Счётчик
};

/**
 * @brief Состояние врага (Хоббина/Ноббина)
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
};
#pragma pack(pop)

/**
 * @brief Поле состояний ячеек фона
 */
uint8_t background[H_MAX][W_MAX];

/**
 * @brief Поле состояний монеток. Установленный бит означает наличие монетки
 */
uint16_t coins[H_MAX];

/**
 * @brief Состояние мешков с деньгами
 */
struct bag_info bags[MAX_BAGS];

/**
 * @brief Состояние врагов (Хоббинов/Ноббинов)
 */
struct bug_info bugs[MAX_BUGS];

// Переменные отвечающие за состояние Диггера
uint16_t man_image_phase;      /// Фаза отображения спрайта Диггера
uint16_t man_image_phase_inc;  /// Инкремент(декремент) фазы отображения спрайта Диггера
uint16_t man_wait;             /// Задержка перед следующим перемещением Диггера
uint16_t man_x_graph;          /// Положение Диггера по оси X в графических координатах
uint16_t man_y_graph;          /// Положение Диггера по оси Y в графических координатах
enum direction man_dir;        /// Направление движения Диггера
enum direction man_prev_dir;   /// Предыдущее направление движения Диггера
enum direction man_new_dir;    /// Желаемое новое направление движения Диггера
enum creature_state man_state; /// Состояние Диггера (жив, убит, лежит дохлый)
struct bag_info *man_dead_bag; /// Указатель на мешок от котрого погиб Диггер

// Переменные отвечающие за создание врагов
uint8_t bugs_max;           /// Максимальное количество врагов на уровне одновременно
uint8_t bugs_total;         /// Общее количество врагов на уровне
uint8_t bugs_delay;         /// Задержка перед рождением врага (Ноббина)
uint8_t bugs_delay_counter; /// Счётчик задержки перед рождением врага
uint8_t bugs_active;        /// Количество активных врагов
uint8_t bugs_created;       /// Общее количество сщзданных врагов

// Переменные отвечающие за мешки
uint8_t broke_max; // Время через которое исчезнет разбившийся мешок

// Переменные отвечающие за бонус-режим
enum bonus_state bonus_state; /// Состояние режима бонус
uint16_t bonus_time;          /// Время активности бонус-режима
uint8_t  bonus_flash;         /// Время мерцания при включении/выключении Бонус-режима
uint8_t  bonus_count;         /// Множитель очков в Бонус-режиме (умножается на два за каждого пойманного врага)
uint32_t bonus_life_score;    /// Количество очков для дополнительное жизни

// Переменные отвечающие за выстрел
uint16_t mis_x_graph;     /// Положение выстрела по оси X в графических координатах
uint16_t mis_y_graph;     /// Положение выстрела по оси Y в графических координатах
uint8_t  mis_image_phase; ///< Фаза анимации при выводе спрайта снаряда
uint8_t  mis_fire;        /// Флаг выстрела
uint8_t  mis_flying;      /// Флаг означающий, что снаряд летит
uint8_t  mis_wait;        /// Задержка готовности выстрела
uint8_t  mis_explode;     /// Счётчик взрывающегося снаряда
enum direction mis_dir;   /// Направление полёта выстрела

// Переменные отвечающие за состояние игры
uint16_t difficulty; /// Уровень сложности
uint16_t level_no;   /// Текущий номер уровня
int16_t  lives;      /// Текущее количество жизней
uint32_t score;      /// Количество очков

// Переменные отвечающие за вывод звуков
uint16_t snd_effects = 1;    /// Флаг, показывающий, что звуковые эффекты включены
uint8_t  chase_snd;          /// Флаг, означающий, что звук включения бонус-режима активн
uint8_t  chase_snd_flip;     /// Флаг переключающий тональность звука при включении/выключении бонус-режима
uint8_t  loose_snd;          /// Флаг, означающий, что звук качающегося мешка включен
uint16_t loose_snd_phase;    /// Фаза звука качающегося мешка
uint8_t  fall_snd;           /// Флаг, означающий, что звук летящего мешка включен
uint8_t  fall_snd_phase;     /// Фаза звука падающего мешка
uint16_t fall_period;        /// Период звука летящего мешка
uint8_t  break_bag_snd;      /// Флаг, означающий, что звук разбивающегося мешка включен
uint8_t  money_snd;          /// Флаг, означающий, что звук съедания золота включен
uint8_t  coin_snd;           /// Флаг, означающий, что звук съедания монетки включен
int8_t   coin_snd_note;      /// Номер ноты при съедании монетки (драгоценного камня)
uint8_t  coin_time;          /// Таймер между последовательными съедениями драгоценных камней (монеток)
uint8_t  fire_snd;           /// Флаг, означающий, что звук выстрела включен
uint16_t fire_snd_period;    /// Период звука выстрела
uint8_t  explode_snd;        /// Флаг, означающий, что звук взрыва включен
uint8_t  done_snd;           /// Флаг, означающий, что звук завершения уровня включен
uint8_t  bug_snd;            /// Флаг, означающий, что звук съедания врага в бонус-режиме включен
uint8_t  life_snd;           /// Флаг, означающий, что звук получения дополнительной жизни включен
uint16_t music_idx;          /// Индекс ноты в фоновой музыке
uint16_t music_count;        /// Счётчик повторов ноты музыки

#if defined(DEBUG)
/**
 * @brief Отладочная процедура отображения мини-карты состояния фона
 */
void draw_bg_minimap()
{
    sp_put(48, SCREEN_Y_OFFSET + MOVE_Y_STEP + 2, sizeof(background[0]), sizeof(background) / sizeof(background[0]), (uint8_t*)background, 0);
}

/**
 * @brief Отладочная процедура отображения мини-карты состояния монеток (драгоценных камней)
 */
void draw_coin_minimap()
{
    sp_put(45, SCREEN_Y_OFFSET + MOVE_Y_STEP + 2, sizeof(coins[0]), sizeof(coins) / sizeof(coins[0]), (uint8_t*)coins, 0);
}
#else
#define draw_bg_minimap() ;
#define draw_coin_minimap() ;
#endif

int remove_coin(uint8_t x_log, uint8_t y_log);

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
 * @brief Вывод количества жизней в виде спрайтов Диггера рядом с количеством очков
 */
void print_lives()
{
    uint16_t man_x_offset = sizeof(ch_digits[0][0]) * 5 + 1; // Смещение шириной в пять символов '0' плюс один байт (4 пикселя)
    constexpr uint16_t man_y_offset = SCREEN_Y_OFFSET + 2; // Смещение спрайта Диггера по оси Y
    constexpr uint16_t one_pos_width = sizeof(image_digger_right[1][0]) + 1; // Ширина спрайта Диггера плюс один байт
    constexpr uint16_t height = sizeof(image_digger_right[1]) / sizeof(image_digger_right[1][0]); // Высота спрайта Диггера
    int16_t width = MAX_LIVES * one_pos_width; //  Общий размер места занимаемый спрайтами Диггера
    uint8_t *sprite = (uint8_t *)image_digger_right[1];

    for (uint16_t l = 1; width > 0; man_x_offset += one_pos_width, width -= one_pos_width)
    {
        if (++l > lives)
        {
            sp_paint_brick(man_x_offset, man_y_offset, width, height, 0);
            break;
        }

        sp_4_15_put(man_x_offset, man_y_offset, sprite);
    }
}

/**
 * @brief Добавление заданного количества очков и вывод очков в левом верхнем углу экрана
 */
void add_score(uint16_t score_add)
{
    score += score_add;
    print_dec(score, 0, SCREEN_Y_OFFSET + MOVE_Y_STEP);

     // Если количество жизней не достигло максимального и количество очков досигло бонусного для получения жизни
    if (lives < MAX_LIVES && (score >= bonus_life_score))
    {
        lives++; // Увеличичить количество жизней на единицу
        print_lives(); // Вывесли количество жизней
        bonus_life_score += BONUS_LIFE_SCORE; // Количество очков до следующего бонуса в виде жизни
        life_snd = 24; // Издать звук получения жизни
    }
}

/**
 * @brief Добавление очков за убитого врага
 */
void add_score_250()
{
    add_score(250); // 250 очков за убитого врага
}

/**
 * @brief Стирает блок 16x15 пикселей (4x15 байт)
 *
 * @param x_graph - координата X блока
 * @param y_graph - координата Y блока
 */
void erase_4_15(uint16_t x_graph, uint16_t y_graph)
{
    sp_paint_brick(x_graph, y_graph, 4, 15, 0);
}


/**
 * @brief Проверка соприкосновения (по расстояниям) по оси X и по оси Y
 */
int check_collision_4_15(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    return (ABS(x2 - x1) < 4) && (ABS(y2 - y1) < 15);
}

/**
 * @brief Получение ячейки уровня по заданным координатам
 */
enum level_symbols getLevelSymbol(uint8_t y_log, uint8_t x_log)
{
    uint8_t byte_no = 0;
    uint8_t rem = x_log;

    while (rem >= 5)
    {
        rem -= 5;
        byte_no++;
    }

    return (level[level_no][y_log][byte_no] >> (rem * 3)) & 7;
}

void bonus_indicator(uint8_t color);

/**
 * @brief Инициализация переменных состояния перед старом уровня
 */
void init_level_state()
{
    bonus_state = BONUS_OFF;

    // Стереть вишенку
    erase_4_15(FIELD_X_OFFSET + (W_MAX - 1) * POS_X_STEP, FIELD_Y_OFFSET);

    // Отключение индикации бонус-режима
    bonus_indicator(0);

    // Деактивировать всех врагов
    for (uint8_t i = 0; i < MAX_BUGS; ++i)
    {
        struct bug_info *bug = &bugs[i]; // Структура с информацией о враге
        if (bug->state == CREATURE_INACTIVE) continue; // Пропустить неактивных врагов

        erase_4_15(bug->x_graph, bug->y_graph); // Стереть врага
        bug->state = CREATURE_INACTIVE; // Деактивировать врага
    }

    // Деактивировать нестационарные мешки
    for (uint16_t i = 0; i < MAX_BAGS; ++i)
    {
        struct bag_info *bag = &bags[i];  // Структура с информацией о мешке
        if (bag->state < BAG_FALLING) continue; // Пропустить неактивные и стационарные мешки

        sp_put(bag->x_graph, bag->y_graph, sizeof(image_bag[0]), sizeof(image_bag) / sizeof(image_bag[0]), 0, (uint8_t *)outline_bag); // Стереть мешок
        // erase_4_15(bag->x_graph, bag->y_graph); // Стереть мешок
        bag->state = BAG_INACTIVE;
    }

    // print_dec(difficulty, 0, MAX_Y_POS + 2 * POS_Y_STEP);

    if (difficulty > 6) bugs_max = 5;      // На уровне сложности 7 и выше максимально 5 врагов одновременно
    else if (difficulty > 0) bugs_max = 4; // На уровне сложности со 1 до 6 (включительно) до 4 врагов одновременно
    else bugs_max = 3;                      // На первом уровне максимально три варага одновременно

    // Переменные относщиеся к созданию и управлению врагами
    bugs_total = difficulty + 6;         // Общее количество врагов на уровне - шесть плюс уровень сложности
    bugs_delay = 45 - (difficulty << 1); // Задержка появления врагов (с ростом сложности убывает)
    bugs_delay_counter = bugs_delay;     // Инициализация счётчика задержки врага исходным значением
    bugs_active = 0;                     // Количество активных врагов
    bugs_created = 0;                    // Общее количество сщзданных врагов

    broke_max = 140 - difficulty * 10; // Время через которое исчезнет разбившийся мешок (с ростом сложности убывает)

    // Инициализация переменных Диггера
    man_dir = DIR_RIGHT;
    man_prev_dir = DIR_RIGHT;
    man_x_graph = FIELD_X_OFFSET + MAN_START_X * POS_X_STEP; // Исходная координата Диггера на экране по оси X
    man_y_graph = FIELD_Y_OFFSET + MAN_START_Y * POS_Y_STEP; // Исходная координата Диггера на экране по оси Y
    man_image_phase = 0;        // Фаза анимации Диггера
    man_image_phase_inc = 1;    // Направление ихменения фазы анимации Диггера
    man_wait = 0;               // Задержка перед следующим перемещением Диггера
    man_state = CREATURE_ALIVE; // Исходное состояние - Диггер жив

    // Инициализация переменных снаряда
    mis_fire = 0;
    mis_flying = 0;
    mis_wait = 0;
    mis_explode = 0;

    // Инициализация переменных используемых для звуковых эффектов
    coin_snd = 0;
    coin_snd_note = -1;
    coin_time = 0;
    fire_snd = 0;
    explode_snd = 0;
    loose_snd = 0;
    fall_snd = 0;
    money_snd = 0;
    chase_snd = 0;
    chase_snd_flip = 0;
    done_snd = 0;
    bug_snd = 0;
    life_snd = 0;
    music_idx = 0;
    music_count = 0;
}

/**
 * @brief Прогрызть фон в соответствии с направлением движения и текущим положением
 *
 * @param dir - направление движения
 * @param x_graph - графическая координата по оси X
 * @param y_graph - графическая координата по оси Y
 */
void gnaw(enum direction dir, uint16_t x_graph, uint16_t y_graph)
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
void init_level()
{
    // Деактивировать все мешки
    for (uint16_t i = 0; i < MAX_BAGS; ++i)
    {
        bags[i].state = BAG_INACTIVE;
    }

    // Деактивировать всех врагов
    for (uint8_t i = 0; i < MAX_BUGS; ++i)
    {
        bugs[i].state = CREATURE_INACTIVE;
    }

    constexpr uint16_t bg_block_width = sizeof(image_background[0][0]); // Ширина блока фона
    constexpr uint16_t bg_block_height = sizeof(image_background[0]) / sizeof(image_background[0][0]); // Высота блока фона

    constexpr uint16_t x_size = 13; // Ширина поля фона в блоках
    constexpr uint16_t y_size = POS_Y_STEP * H_MAX / bg_block_height + MOVE_Y_STEP + 2; // Высота поля фона в блоках

    const uint8_t *back_image = (uint8_t *)image_background[level_no]; // Указатель на образец фона для текущего уровня

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
                struct bag_info *bag = &bags[bag_num++]; // Структура с информацией об очередном мешке

                bag->state = BAG_STATIONARY; // Мешок стоит на месте
                bag->count = 0;              // Счётчик сброшен
                bag->x_graph = x_graph;      // Координата мешка по оси X
                bag->y_graph = y_graph;      // Координата мешка по оси Y
                bag->dir = DIR_STOP;         // Мешок стоит на месте

                // Нарисовать мешок с золотом
                sp_put(bag->x_graph, bag->y_graph, sizeof(image_bag[0]), sizeof(image_bag) / sizeof(image_bag[0]), (uint8_t *)image_bag, (uint8_t *)outline_bag);
            }
            else if (ls == LEV_H || ls == LEV_S)
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
 * @brief Определяет по состоянию байта клетки, что клетка полностью проедена
 *
 * @param byte - байт с состоянием клетки
 *
 * @return - 1 - клетка полностью проедена, 0 - клетка не проедена полностью
 */
uint16_t full_bite(uint8_t byte)
{
    int bits_count;
    for (bits_count = 0; byte; bits_count++)
    {
        byte &= byte - 1;
    }

    return bits_count > 1;
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
uint8_t check_path(enum direction dir, uint8_t x_graph, uint8_t y_graph)
{
    const uint8_t abs_x_pos = x_graph - FIELD_X_OFFSET;
    const uint8_t abs_y_pos = y_graph - FIELD_Y_OFFSET;
    uint8_t x_log = abs_x_pos / POS_X_STEP;
    uint8_t y_log = abs_y_pos / POS_Y_STEP;
    const uint8_t current_cell = background[y_log][x_log];

    static const struct
    {
        int8_t  x;
        int8_t  y;
        uint8_t mask;
        uint8_t cur_mask;
    } dir_matrix[4] = {
        { -1,  0, 0x08, 0x01 },
        {  1,  0, 0x01, 0x08 },
        {  0, -1, 0x80, 0x10 },
        {  0,  1, 0x10, 0x80 }
    } ;

    x_log += dir_matrix[dir].x;
    y_log += dir_matrix[dir].y;

    if ((x_log >= W_MAX) || (y_log >= H_MAX)) return 0;

    const uint8_t neighbor_cell = background[y_log][x_log];
    if (full_bite(neighbor_cell) && ((neighbor_cell & dir_matrix[dir].mask) || (current_cell & dir_matrix[dir].cur_mask))) return 1;

    return 0;
}

/**
 * @brief Очистить биты состоянияфона по заданным координатам в указанном направлении
 *
 * @param x_graph - графическая координата по оси X
 * @param y_graph - графическая координата по оси Y
 * @param dir - направлкние движения
 */
void clear_background_bits(uint16_t x_graph, uint16_t y_graph, enum direction dir)
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
int check_out_of_range(enum direction dir, uint16_t x_graph, uint16_t y_graph)
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
int move_to_object(enum direction dir, uint16_t x_graph, uint16_t object_x_graph)
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
uint8_t move_bag(struct bag_info *bag, enum direction dir)
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
        if (check_collision_4_15(bag_x_graph, bag_y_graph, man_x_graph, man_y_graph))
        {
            if (move_to_object(dir, bag_x_graph, man_x_graph))
            {
                rv = 1; // Если да, отменить перемещение
            }
        }

        // Проверить перемещается ли мешок на врага
        for (uint8_t i = 0; i < bugs_max; ++i)
        {
            struct bug_info *bug = &bugs[i]; // Структура с информацией о враге
            if (!bugs_active) continue; // Пропустить неактивных врагов
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
            struct bag_info *another_bag = &bags[i]; // Структура с информацией о мешке

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
        uint8_t bag_abs_x_pos = bag_x_graph - FIELD_X_OFFSET;
        uint8_t bag_abs_y_pos = bag_y_graph - FIELD_Y_OFFSET;
        uint8_t bag_x_log = bag_abs_x_pos / POS_X_STEP;
        uint8_t bag_y_log = bag_abs_y_pos / POS_Y_STEP;

        // Стирание мешка по старым координатам
        sp_put(bag->x_graph, bag->y_graph, 4, 15, nullptr, (uint8_t *)outline_bag);

        // Отрисовка спрайта передвигаемого мешка
        sp_put(bag_x_graph, bag_y_graph, 4, 15, (uint8_t *)image_bag, (uint8_t *)outline_bag);

        clear_background_bits(bag_x_graph, bag_y_graph, dir); // Сбросить биты матрицы фона
        remove_coin(bag_x_log, bag_y_log); // Удалить монеты уничтоженные мешком

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
void erase_trail(enum direction dir, uint16_t x_graph, uint16_t y_graph)
{
    static const struct
    {
        int16_t x;
        int16_t y;
        uint8_t x_size;
        uint8_t y_size;
    } dir_matrix[4] = {
        { 4, 0, MOVE_X_STEP, 15            },
        { -MOVE_X_STEP, 0, MOVE_X_STEP, 15 },
        { 0, 15, 4, MOVE_Y_STEP            },
        { 0, -MOVE_Y_STEP, 4, MOVE_Y_STEP  }
    } ;

    x_graph += dir_matrix[dir].x;
    y_graph += dir_matrix[dir].y;

    sp_paint_brick(x_graph, y_graph, dir_matrix[dir].x_size, dir_matrix[dir].y_size, 0);
}

/**
 * @brief Обработка перемещения Ноббина/Хоббина
 *
 * @param bug - указатель на структуру с информацией о враге
 */
void move_bug(struct bug_info *bug)
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
        if ((bug->type == BUG_HOBBIN) && (bug->count > (32 + difficulty * 2)))
        {
            bug->count = 0;         // Очистить время застревания
            bug->type = BUG_NOBBIN; // Превратить врага в Ноббина
        }

        // Поиск порядка наилучших направлений движения
        if (ABS(man_y_graph - bug_y_graph) > ABS(man_x_graph - bug_x_graph))
        {
            // Если расстояние по вертикали превышает расстояние по горизонтали, то отдать приоритет оси Y
            if (man_y_graph < bug_y_graph)
            { // Если Диггер выше врага
                dir_1 = DIR_UP;   // Наиболее приоритетное направление - вверх
                dir_4 = DIR_DOWN; // Наименее приоритетное направление - вниз
            }
            else
            { // Если Диггер ниже врага
                dir_1 = DIR_DOWN; // Наиболее приоритетное направление - вниз
                dir_4 = DIR_UP;   // Наименее приоритетное направление - вверх
            }

            if (man_x_graph < bug_x_graph)
            { // Если диггер левее врага
                dir_2 = DIR_LEFT;  // Более приоритетное направление - влево
                dir_3 = DIR_RIGHT; // Менее приоритетное направление - вправо
            }
            else
            { // Если Диггер правее врага
                dir_2 = DIR_RIGHT; // Более приоритетное направление - вправо
                dir_3 = DIR_LEFT;  // Менее приоритетное направление - влево
            }
        }
        else
        {
            // Если расстояние по горизонтали превышает расстояние по вертикали, то отдать приоритет оси X
            if (man_x_graph < bug_x_graph)
            { // Если диггер левее врага
                dir_1 = DIR_LEFT;  // Наиболее приоритетное направление - влево
                dir_4 = DIR_RIGHT; // Наименее приоритетное направление - вправо
            }
            else
            { // Если Диггер правее врага
                dir_1 = DIR_RIGHT; // Наиболее приоритетное направление - вправо
                dir_4 = DIR_LEFT;  // Наименее приоритетное направление - влево
            }

            if (man_y_graph < bug_y_graph)
            { // Если Диггер выше врага
                dir_2 = DIR_UP;   // Более приоритетное направление - вверх
                dir_3 = DIR_DOWN; // Менее приоритетное направление - вниз
            }
            else
            { // Если Диггер ниже врага
                dir_2 = DIR_DOWN; // Более приоритетное направление - вниз
                dir_3 = DIR_UP;   // Менее приоритетное направление - вверх
            }
        }

        // Если включён режим Бонус, то поменять порядок направлений чтобы враги разбегались
        if (bonus_state == BONUS_ON)
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
        if ((difficulty < 5) && ((rand() & 0xF) > (difficulty + 10)))
        {
            // В одном из (5 + difficulty) случаев поменять наиболее
            // приоритетное направление с менее приоритетным
            dir_1 ^= dir_3;
            dir_3 ^= dir_1;
            dir_1 ^= dir_3;
        }

        if (bug->type == BUG_NOBBIN)
        {
            // Для Ноббинов нужно выбрать наилучшее направление по которому свободен путь
                 if (check_path(dir_1, bug_x_graph, bug_y_graph)) { dir = dir_1; }
            else if (check_path(dir_2, bug_x_graph, bug_y_graph)) { dir = dir_2; }
            else if (check_path(dir_3, bug_x_graph, bug_y_graph)) { dir = dir_3; }
            else if (check_path(dir_4, bug_x_graph, bug_y_graph)) { dir = dir_4; }
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
        clear_background_bits(bug_x_graph, bug_y_graph, bug->dir);

        // Стерерь кусочек фона на экране в соответствии с направлением движения и текущим положением
        gnaw(bug->dir, bug_x_graph, bug_y_graph);

        uint8_t bug_x_log = bug_abs_x_pos / POS_X_STEP;
        uint8_t bug_y_log = bug_abs_y_pos / POS_Y_STEP;

        remove_coin(bug_x_log, bug_y_log);
    }

    if (man_state == CREATURE_ALIVE) // Если Диггер жив
    {
        // Выждать время задержки перед запуском нового врага
        if (bug->state == CREATURE_STARTING)
        {
            if (bug->count > 0) bug->count--;
            else bug->state = CREATURE_ALIVE; // Если счётчик закончился, что оживить врага
        }
        else
        {
            // Переместить врага на шаг в выбранном направлении
            static const struct
            {
                int8_t  x;
                int8_t  y;
            } dir_matrix[4] = {
                { -MOVE_X_STEP, 0 },
                {  MOVE_X_STEP, 0 },
                { 0, -MOVE_Y_STEP },
                { 0,  MOVE_Y_STEP }
            } ;

            bug->x_graph += dir_matrix[bug->dir].x;
            bug->y_graph += dir_matrix[bug->dir].y;
        }
    }

    if (bug->state == CREATURE_ALIVE)
    {
        // Проверить соприкосновение врага с мешками
        for (uint8_t i = 0; i < MAX_BAGS; ++i)
        {
            struct bag_info *bag = &bags[i]; // Структура с информацией о мешках

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
                        {
                            enum direction dir = bug->dir;

                            // Если Ноббин движется влево или вправо
                            if ((dir == DIR_LEFT) || (dir == DIR_RIGHT))
                            {
                                if (move_bag(bag, dir)) // Попытаться переместить мешок
                                {
                                    // Если мешок не удалось подвинуть, отменить передвижение врага
                                    bug->x_graph = bug_x_graph;
                                    bug->y_graph = bug_y_graph;
                                    bug->count++; // Увеличить счётчик застревания
                                    break;
                                }

                                bug->wait++; // Задержать врага перед мешком
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
                    sp_put(bag->x_graph, bag->y_graph, sizeof(outline_bag_fall[0]), sizeof(outline_bag_fall) / sizeof(outline_bag_fall[0]), nullptr, (uint8_t *)outline_bag_fall);

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
        // Для Ноббина
        sp_4_15_put(bug->x_graph, bug->y_graph, (uint8_t *)image_nobbin[bug->image_phase]);
    }
    else
    {
        // Для Хоббина
        if (bug->dir == DIR_RIGHT)
        {
            // Смотрит вправо
            sp_4_15_put(bug->x_graph, bug->y_graph, (uint8_t *)image_hobbin_right[bug->image_phase]);
        }
        else
        {
            // Смотрит влево (отзеркаленный правый)
            sp_4_15_h_mirror_put(bug->x_graph, bug->y_graph, (uint8_t *)image_hobbin_right[bug->image_phase]);
        }
    }
}

/**
 * @brief Остановка мешка
 *
 * @param bag - указатель на структуру с информацией о мешке
 */
void stop_bag(struct bag_info *bag)
{
    // Если мешок пролетел больше одного этажа, то он будет разбит
    bag->state = (bag->count > 1) ? BAG_BREAKS : BAG_STATIONARY;
    bag->dir = DIR_STOP; // Остановить мешок
    bag->count = 0;

    // TODO: Унчтожить все мешки под тем, который остановился
    for (uint16_t i = 0; i < MAX_BAGS; ++i)
    {
        struct bag_info *another_bag = &bags[i];  // Структура с информацией о мешке
        if (another_bag == bag) continue; // Пропустить мешок обработка которого производится
        if (another_bag->state == BAG_INACTIVE) continue; // Пропустить неактивные мешки

        if (check_collision_4_15(bag->x_graph, bag->y_graph, another_bag->x_graph, another_bag->y_graph))
        {
            // erase_4_15(another_bagbag->x_graph, another_bag->y_graph); // Стереть мешок
            another_bag->state = BAG_INACTIVE;
            break_bag_snd = 1;
        }
    }
}

/**
 * @brief Подпрограмма отрисовки Диггера
 */
void draw_man()
{
    uint8_t cab = !mis_flying && !mis_wait; // Флаг наличия "башенки"

    man_image_phase += man_image_phase_inc; // Переключить фазу изображения

    // При необходимости, сменить направление изменения фазы спрайта
    if (!man_image_phase || man_image_phase >= 2) man_image_phase_inc =- man_image_phase_inc;

    uint16_t image_phase = man_image_phase + ((cab) ? 0 : 3);
    const uint8_t *image = (man_dir < DIR_UP) ? (uint8_t *)image_digger_right[image_phase] : (uint8_t *)image_digger_up[image_phase];

    if (man_dir == DIR_LEFT)
    {
        // Едет влево (отзеркаленный правый)
        sp_4_15_h_mirror_put(man_x_graph, man_y_graph, image);
    }
    else if (man_dir == DIR_RIGHT || man_dir == DIR_UP)
    {
        // Едет вправо или вверх (обычный спрайт)
        sp_4_15_put(man_x_graph, man_y_graph, image);
    }
    else
    {
        // Едет вниз (отзеркален горизонтально и вертикально)
        sp_4_15_hv_mirror_put(man_x_graph, man_y_graph, image);
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
int remove_coin(uint8_t x_log, uint8_t y_log)
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

        if (level_done) done_snd = 1; // Если все камешки съедены, то включить звук окончания уровня

        return 1;
    }

    return 0;
}

/**
 * @brief Подпрограмма обработки звуковых эффектов
 */
void sound_effect()
{
    if (coin_snd) // Звук съедания монеты (драгоценного камня)
    {
        static const uint16_t coin_periods[] = { C5, D5, E5, F5, G5, A5, B5, C6 };
        uint16_t period = coin_periods[coin_snd_note & 7];
        sound(period, 30);
        coin_snd--;
    }

    if (fire_snd)
    {
        fire_snd_period += fire_snd_period >> 2;
        if (fire_snd_period > 800) fire_snd = 0;
        else
        {
            uint16_t period = fire_snd_period + (rand() & (fire_snd_period >> 2));
            sound(period, 10);
        }
    }

    if (explode_snd)
    {
        explode_snd = 0;

        uint16_t explode_snd_period = 1500 / N; // Начальный период звука взрыва

        for (uint16_t i = 10; i != 0; --i)
        {
            explode_snd_period -= explode_snd_period >> 3;
            sound(explode_snd_period, 30);
        }
    }

    if (loose_snd) // Звук раскачивающегося мешка
    {
        static const uint16_t loose_periods[] = { 2500 / N, 3000 / N, 2500 / N, 2000 / N };
        static const uint16_t loose_durances[] = { 24, 20, 24, 30 };

        if (!(loose_snd_phase & 1))
        {
            uint16_t index = loose_snd_phase >> 1;
            sound(loose_periods[index], loose_durances[index]);
        }

        if (++loose_snd_phase > 7) loose_snd_phase = 0;
    }

    if (fall_snd) // Звук падающего мешка
    {
        fall_snd_phase = ~fall_snd_phase;

        if (fall_snd_phase) sound(fall_period / 32, 16);
        else fall_period += 48;
    }

    if (break_bag_snd) // Звук разбивающегося мешка
    {
        sound(15000 / N, 10);
        break_bag_snd = 0;
    }

    if (money_snd) // Звук съедаемого золота
    {
        uint16_t money_snd_count_1 = 500 / N;
        uint16_t money_snd_count_2 = 4000 / N;

        for (uint16_t i = 0; i < 30; ++i)
        {
            uint16_t period = !(i & 1) ? money_snd_count_1 : money_snd_count_2;
            money_snd_count_1 += money_snd_count_1 >> 4;
            money_snd_count_2 -= money_snd_count_2 >> 4;
            sound(period, 25);
        }

        money_snd = 0;
    }

    // if (chase_snd) // Звук включения бонус-режима
    // {
    //     uint16_t durance = 75;
    //     chase_snd_flip = ~chase_snd_flip;
    //     if (chase_snd_flip) sound(1230 / N, durance);
    //     else sound(1513 / N, durance);
    // }

    // if (done_snd) // Звук завершения уровня
    // {
    //     static const uint8_t done_periods[] = { C5 / NV, E5 / NV, G5 / NV, D5 / NV, F5 / NV, A5 / NV, E5 / NV, G5 / NV, B5 / NV, C5 / 2 / NV};
    //
    //     for (uint16_t i = 0; i < sizeof(done_periods) / sizeof(done_periods[0]); ++i)
    //     {
    //         uint8_t period = done_periods[i];
    //         uint16_t durance = (i == 9) ? 800 : 300;
    //         sound_vibrato(period, durance);
    //         delay_ms(2);
    //     }
    // }

    if (bug_snd) // Звук съедаемого врага в бонус-режиме
    {
        uint16_t c1 = 0;
        uint16_t c2 = 4;
        uint16_t bug_period = 0;

        while (c2)
        {
            uint16_t period;
            if (c1)
            {
                uint16_t c1_lb = c1 & 3;
                if (c1_lb == 0) period = bug_period;
                if (c1_lb == 2) period = bug_period - (bug_period >> 4);
                c1--;
                bug_period -= bug_period >> 4;
                sound(period / 16, 25);
            }
            else
            {
                bug_period = 1600;
                c1 = 20;
                c2--;
            }
        }

        bug_snd = 0;
    }

    if (life_snd) // Звук получения жизни
    {
        sound(14 + life_snd, 40);
        life_snd -= 2;
    }
}

/**
 * @brief Инициализация игры
 */
void init_game()
{
    difficulty = 0; // Начальный уровень сложности
    level_no = 0;   // Начальный уровень
    lives = 3;      // Начальное количество жизней
    score = 0;      // Начальное количество очков
    bonus_life_score = BONUS_LIFE_SCORE;

    add_score(0);  // Печать начального количества очков (нули)
    print_lives(); // Вывод начального количества жизней
    init_level();  // Начальная инициализация уровня
}

/**
 * @brief Обработка появления и перемещения врагов (Ноббинов и Хоббинов)
 */
void process_bugs()
{
    // Обработка появления врагов
    if (bugs_delay_counter > 0) --bugs_delay_counter; // Отсчёт времени до появления нового врага
    else
    {
        bugs_delay_counter = bugs_delay; // Перезарядить счётчик времени до появления врага

        if (man_state == CREATURE_ALIVE) // Если Диггер жив
        {
            if ((bugs_created < bugs_total) && (bugs_active < bugs_max)) // Если врагов на экране одновременно меньше максимального количества
            {
                if (bonus_state != BONUS_ON) // Если не включен бонус-режим, запустить нового врага
                {
                    // Координаты рожденияя врагов - в правом верхнем углу
                    constexpr uint8_t bug_start_x = FIELD_X_OFFSET + (W_MAX - 1) * POS_X_STEP;
                    constexpr uint8_t bug_start_y = FIELD_Y_OFFSET + 0 * POS_Y_STEP;

                    for (uint16_t i = 0; i < bugs_max; ++i)
                    {
                        struct bug_info *bug = &bugs[i];

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

                        bugs_active++;  // Увеличить счётчик активных врагов
                        bugs_created++; // Увеличить общее количество созданных врагов

                        break;
                    }
                }
            }
            else
            {
                // Если Бонус (вишенка) ещё не появлялся и создано максимальное количество врагов
                if ((bonus_state == BONUS_OFF) && (bugs_created == bugs_total))
                {
                    bonus_state = BONUS_READY; // Включить готовность к активации бонус-режима
                }
            }
        }
    }

    // Обработка врагов (Ноббинов и Хоббинов)
    for (uint16_t i = 0; i < bugs_max; ++i)
    {
        struct bug_info *bug = &bugs[i]; // Структура с информацией о враге

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
                    for (uint16_t t = 0; t < bugs_max; ++t)
                    {
                        if (t == i) continue; // Пропустить самого себя

                        struct bug_info *another_bug = &bugs[t];
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
                    if (bug->count > (20 - difficulty))
                    {
                        bug->count = 0;         // Сбросить счётчик застревания
                        bug->type = BUG_HOBBIN; // Переключить тип врага на Хоббина
                    }
                }

                // Если выпало случайное число с вероятностью зависящей от уровня сложности
                if ((rand() & 0xF) < difficulty) move_bug(bug); // Переместить врага ещё раз для увеличения скорости

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
                bugs_active--;                          // Уменьшить количество активных врагов

                // Количество оставшихся врагов (сколько осталось создать плюс количество активных)
                uint8_t creatures_left =  bugs_total - bugs_created + bugs_active;
                if (!creatures_left) done_snd = 1; // Если врагов больше не осталось - окончание уровня

                break;
            }
        }
    }
}

/**
 * @brief Обработка мешков
 */
void process_bags(const uint8_t man_x_log, const uint8_t man_y_log)
{
    // Обработка мешков
    uint8_t bags_fall = 0;  // Флаг, показывающий, что присутствуют падающие мешки
    uint8_t bags_loose = 0; // Флаг, показывающий, что присутствуют качающиеся мешки

    for (uint8_t i = 0; i < MAX_BAGS; ++i)
    {
        struct bag_info *bag = &bags[i]; // Структура с информацией о мешке

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
                    if ((bag_y_log != H_MAX - 1) && background[bag_y_log + 1][bag_x_log])
                    {
                        switch (bag->dir)
                        {
                            case DIR_STOP: // Если мешок неподвижен
                            {
                                // Если Диггер двигался вверх и он находится под мешком, то пока не начинать раскачивать мешок
                                if ((man_x_log == bag_x_log) && (man_y_log == bag_y_log + 1) && man_new_dir != DIR_UP)
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
                                fall_snd = 1;             // Включить звук падения мешка

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
                    if (!loose_snd) // Если звук раскачивания мешка ещё не включен
                    {
                        // Включить звук раскачивания мешка
                        loose_snd = 1;
                        loose_snd_phase = 0;
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
                        sp_put(bag_x_graph, bag_y_graph, sizeof(image_bag[0]), sizeof(image_bag) / sizeof(image_bag[0]), bag_image, bag_outline);
                    }
                }
                else
                {
                    // Если счётчик времени до падения мешка закончился
                    bag->state = BAG_FALLING; // Начать падение мешка
                    bag->dir = DIR_DOWN;      // Направление движения мешка - вниз
                    bag->count = 0;           // Сбросить счётчик этажей
                    fall_snd = 1;             // Включить звук падения мешка
                }

                break;
            }

            case BAG_FALLING: // Мешок падает
            {
                bags_fall = 1; // Найден падающий мешок

                // Прогрызть фон и сбросить биты матрицы фона
                gnaw(DIR_UP, bag_x_graph, bag_y_graph + 9);
                clear_background_bits(bag_x_graph, bag_y_graph, DIR_DOWN); // Сбросить биты матрицы фона
                clear_background_bits(bag_x_graph, bag_y_graph - 1, DIR_DOWN); // Сбросить биты матрицы фона

                // Стереть падающий мешок по старым координатам
                if (bag->count) // Если номер этажа не нулевой
                {
                    // Если пролетел больше одного этажа, то стираем прямоугольником
                    erase_4_15(bag->x_graph, bag->y_graph);
                }
                else
                {
                    // В начале полёта стираем при помощи маски
                    sp_put(bag->x_graph, bag->y_graph, sizeof(outline_bag_fall[0]), sizeof(outline_bag_fall) / sizeof(outline_bag_fall[0]), nullptr, (uint8_t *)outline_bag_fall);
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
                    if ((bag_y_log == H_MAX - 1) || !full_bite(background[bag_y_log + 1][bag_x_log])) stop_bag(bag);
                }

                remove_coin(bag_x_log, bag_y_log); // Удалить монету в клетке куда попал мешок

                // TODO: Удалить мешки встречающиеся на пути (dest_coin(x, y))

                // Нарисовать падающий мешок
                sp_4_15_put(bag_x_graph, bag_y_graph, (uint8_t *)image_bag_fall);
                // sp_put(bag_x_graph, bag_y_graph, sizeof(image_bag_fall[0]), sizeof(image_bag_fall) / sizeof(image_bag_fall[0]),
                //         (uint8_t *)image_bag_fall, (uint8_t *)outline_bag_fall);

                if (man_state == CREATURE_ALIVE) //  Если Диггер жив
                {
                    // Проверить, что Диггер попал под падающий под мешок
                    if (check_collision_4_15(man_x_graph, man_y_graph, bag_x_graph, bag_y_graph))
                    {
                        man_state = CREATURE_DEAD_MONEY_BAG; // Диггер погиб от падающего мешка
                        man_dead_bag = bag; // Указатель на мешок от которого погиб Диггер
                    }
                }

                // Попытаться спасти врагов от падающего мешка
                for (uint8_t i = 0; i < bugs_max; ++i)
                {
                    volatile struct bug_info *bug = &bugs[i]; // Структура с информацией о враге
                    if (bug->state != CREATURE_ALIVE) continue; // Пропустить неживых врагов

                    uint8_t bug_x_graph = bug->x_graph;
                    uint8_t bug_abs_x_pos = bug_x_graph - FIELD_X_OFFSET;
                    uint8_t bug_x_log = bug_abs_x_pos / POS_X_STEP;

                    // Сделать чтобы враги пытались убежать от летящего мешка
                    // Если враг находится на одной вертикальной линии с мешком и
                    // движется вверх, то изменить направление движения на движение вниз
                    if (bug_x_log == bag_x_log && bug->dir == DIR_UP) bug->dir = DIR_DOWN;

                    // Проверить, что враг попал под падающий мешок
                    if (check_collision_4_15(bug->x_graph, bug->y_graph, bag_x_graph, bag_y_graph + 8))
                    {
                        bug->state = CREATURE_DEAD_MONEY_BAG; // Враг был убит мешком с деньгами

                        // В бонус-режиме увеличить количество создаваемых врагов компенсируя убитых мешками
                        if (bonus_state == BONUS_ON)
                        {
                            bugs_total++; // Увеличить количество создаваемых врагов
                            bugs_active--; // Уменьшить количество активных врагов
                        }
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
                        break_bag_snd = 1; // Издать звук разбившегося мешка
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

    if (fall_snd) // Если звук падения мешков включен
    {
        if (!bags_fall) // Но, мешков падающих нет
        {
            fall_snd = 0; // Выключить звук падения мешков
        }
    }
    else
    {
        if (bags_fall) // Если есть падающие мешки
        {
            // Включить звук падения мешков
            fall_period = 1024;
            fall_snd_phase = 0;
            fall_snd = 1;
        }
    }

    if (!bags_loose) // Если ни один мешок не качается
    {
        loose_snd = 0; // Выключить звук качающегося мешка
    }
}

/**
 * @brief Обработка выстрела
 */
void process_missile()
{
    // Размеры и количество фаз анимации выстрела
    constexpr uint16_t missile_x_size = sizeof(image_missile[0][0]);
    constexpr uint16_t missile_y_size = sizeof(image_missile[0]) / missile_x_size;
    constexpr uint16_t missile_phases_no = sizeof(image_missile) / sizeof(image_missile[0]);

    // Размеры и количество фаз анимации взрыва
    constexpr uint16_t explode_x_size = sizeof(image_explode[0][0]);
    constexpr uint16_t explode_y_size = sizeof(image_explode[0]) / explode_x_size;
    constexpr uint16_t explode_phases_no = sizeof(image_explode) / sizeof(image_explode[0]);

    if (mis_explode)
    {
        // Обработка взрывающегося выстрела
        if (mis_image_phase < explode_phases_no)
        {
            // Вывести изображение взрыва
            sp_put(mis_x_graph, mis_y_graph, explode_x_size, explode_y_size, (uint8_t *)image_explode[mis_image_phase++], nullptr);
        }
        else
        {
            // Стереть изображение взрыва
            sp_paint_brick(mis_x_graph, mis_y_graph, explode_x_size, explode_y_size, 0);
            mis_flying = 0;  // Выстрел больше не летит
            mis_explode = 0; // И не взрывается
        }
    }
    else
    {
        // Обработка летящего выстрела
        if (mis_flying)
        {
            // Стереть предыдущее изображение выстрела
            // sp_put(mis_x_graph, mis_y_graph, missile_x_size, missile_y_size, nullptr, (uint8_t *)outline_missile);
            sp_paint_brick(mis_x_graph, mis_y_graph, missile_x_size, missile_y_size, 0);

            // Переместить выстрел на один шаг в заданном направлении
            static const struct
            {
                int8_t  x;
                int8_t  y;
            } dir_matrix[4] = {
                { -2 * MOVE_X_STEP, 0  },
                {  2 * MOVE_X_STEP, 0  },
                {  0, -2 * MOVE_Y_STEP },
                {  0,  2 * MOVE_Y_STEP }
            } ;

            mis_x_graph += dir_matrix[mis_dir].x;
            mis_y_graph += dir_matrix[mis_dir].y;

            uint8_t explode = 0;

            // Проверить если координаты выходят за рамки игрового поля или впереди нету прохода

            if (!check_path(mis_dir, mis_x_graph, mis_y_graph))
            {
                explode = 1; // Взорвать выстрел
            }
            else
            {
                // Циклически менять фазу анимации выстрела
                if (++mis_image_phase >= missile_phases_no) mis_image_phase = 0;

                // Вывести новое изображение выстрела
                sp_put(mis_x_graph, mis_y_graph, missile_x_size, missile_y_size, (uint8_t *)image_missile[mis_image_phase], nullptr);

                // Проверить попал ли выстрел во врага
                for (uint8_t i = 0; i < bugs_max; ++i)
                {
                    struct bug_info *bug = &bugs[i]; // Структура с информацией о враге
                    if (bug->state != CREATURE_ALIVE) continue; //  Пропустить неживых врагов

                    // Проверить, что выстрел попал во врага
                    if (check_collision_4_15(mis_x_graph, mis_y_graph, bug->x_graph, bug->y_graph))
                    {
                        explode = 1; // Взорвать выстрел
                        bug->state = CREATURE_RIP; // Враг был убит выстрелом
                        add_score_250(); // Добавить 250 очков за убитого врага

                        // В бонус-режиме увеличить количество создаваемых врагов компенсируя убитых мешками
                        if (bonus_state == BONUS_ON)
                        {
                            bugs_total++; // Увеличить количество создаваемых врагов
                            bugs_active--; // Уменьшить количество активных врагов
                        }
                    }
                }
            }

            if (explode)
            {
                fire_snd = 0;                  // Выключить звук летящего выстрела
                mis_explode = 1;               // Включить взрыв выстрела
                mis_image_phase = 0;           // Начальная фаза взрыва
                explode_snd = 1;               // Включить звук взрыва
            }
        }
        else
        {
            if (mis_wait) mis_wait--; // Уменьшить счётчик задержки выстрела
            else
            {
                if (mis_fire) // Если произведён выстрел
                {
                    mis_fire = 0;
                    mis_wait = 85 + difficulty * 4; // Начальное значение счётчика появления "башенки"
                    mis_image_phase = 0;
                    mis_flying = 1;
                    mis_dir = man_dir;

                    // Определить начальное положение выстрела в зависимости от
                    // координат Диггера и его направления движения
                    static const struct
                    {
                        int16_t  x;
                        int16_t  y;
                    } dir_matrix[4] = {
                        { -MOVE_X_STEP, MOVE_Y_STEP },
                        {  4,  MOVE_Y_STEP },
                        {  1, -MOVE_Y_STEP },
                        {  1,  15 + MOVE_Y_STEP }
                    } ;

                    mis_x_graph = man_x_graph + dir_matrix[mis_dir].x;
                    mis_y_graph = man_y_graph + dir_matrix[mis_dir].y;

                    // Вывести начальное положение спрайта выстрела
                    sp_put(mis_x_graph, mis_y_graph, missile_x_size, missile_y_size, (uint8_t *)image_missile[mis_image_phase], nullptr);

                    // Включить звук выстрела
                    fire_snd_period = 10;
                    fire_snd = 1;
                }
            }
        }
    }
}

void man_rip();

/**
 * @brief Съесть монету (драгоценный камень)
 */
inline void eat_coin()
{
    coin_snd = 7;  // Включить звук съедения монеты
    coin_time = 9; // Взвести таймер до последующего съедения монеты
    add_score(25); // 25 очков за съеденную монету (камешек)
    if (++coin_snd_note == 7) // Перейти к следующей ноте
    {
        coin_snd_note = -1;
        add_score_250(); // Добавить 250 очков за съедение восьми последовательных монет
    }
}

/**
 * @brief Обработка Диггера
 */
void process_man(const uint8_t man_x_rem, const uint8_t man_y_rem)
{
    // Обработка перемещения Диггера
    if (man_state == CREATURE_ALIVE) // Если Диггер жив
    {
        if (man_wait) man_wait--; // Если Диггер в режиме задержки (при толкании мешков)
        else
        {
            man_new_dir = DIR_STOP;

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
                        man_new_dir = joy_dirs[i];
                        break;
                    }
                }
            }

            if (!mis_wait && (port_state & ((1 << PAR_INTERF_LEFT_BUTTON) | (1 << PAR_INTERF_RIGHT_BUTTON)))) mis_fire = 1;

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
                        if (!mis_wait) mis_fire = 1;
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
                        if (++difficulty >= 10) difficulty = 0;
                        break;
                    }

                    case 'L':  // Добавление жизни
                    {
                        lives++;
                        print_lives();
                        life_snd = 24;
                        break;
                    }

                    case 'N':  // Переход на следующий уровень
                    {
                        done_snd = 1;
                        break;
                    }
#endif
                }
            }

            // Если новое желаемое направление движения вверх-вниз, то применить его в середине клетки по-горизонтали
            if (man_x_rem == 0 && (man_new_dir == DIR_UP || man_new_dir == DIR_DOWN))
            {
                man_dir = man_new_dir;
            }

            // Если новое желаемое направление движения влево-вправо, то применить его в середине клетки по-вертикали
            if (man_y_rem == 0 && (man_new_dir == DIR_LEFT || man_new_dir == DIR_RIGHT))
            {
                man_dir = man_new_dir;
            }

            // Остановиться при попытке выхода за игровое поле
            if ((man_new_dir == DIR_STOP) || check_out_of_range(man_dir, man_x_graph, man_y_graph))
            {
                man_dir = DIR_STOP;
            }

            if (bonus_state == BONUS_READY)
            {
                // Проверить что Диггер соприкоснулся с вишенкой
                if (check_collision_4_15(man_x_graph, man_y_graph, FIELD_X_OFFSET + (W_MAX - 1) * POS_X_STEP, FIELD_Y_OFFSET))
                {
                    bonus_state = BONUS_ON; // Включить Бонус-режим
                    bonus_count = 1; // Начальное значение множителя очков в Бонус-режиме
                    bonus_time = 230 - difficulty * 20; // Время действия Бонус-режима
                    bonus_flash = 19; // Время мигания индикатора включения Бонус-режима

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

            uint8_t prev_man_x_graph = man_x_graph;
            uint8_t prev_man_y_graph = man_y_graph;

            if (man_dir != DIR_STOP)
            {
                // Переместить Диггера на один шаг в заданном направлении
                static const struct
                {
                    int16_t  x;
                    int16_t  y;
                } dir_matrix[4] = {
                    { -MOVE_X_STEP, 0 },
                    {  MOVE_X_STEP, 0 },
                    { 0, -MOVE_Y_STEP },
                    { 0,  MOVE_Y_STEP }
                } ;

                man_x_graph += dir_matrix[man_dir].x;
                man_y_graph += dir_matrix[man_dir].y;
            }
            else man_dir = man_prev_dir;

            uint16_t collision_flag = 0;

            // Обработка толкания мешков и съедения золота
            for (uint8_t i = 0; i < MAX_BAGS; ++i)
            {
                struct bag_info *bag = &bags[i]; // Структура с информацией о мешке
                if (bag->state == BAG_INACTIVE) continue; // Пропустить неактивные мешки

                // Если Диггер не соприкоснулся с мешком, проверить следующий мешок
                if (!check_collision_4_15(bag->x_graph, bag->y_graph, man_x_graph, man_y_graph)) continue;

                man_wait++; // Задержать Диггера перед мешком или золотом

                switch (bag->state)
                {
                    case BAG_STATIONARY:
                    case BAG_LOOSE:
                    {
                        // Если направление движения Диггера вверх или вниз, или мешок не удалось переместить
                        if (man_dir == DIR_UP || man_dir == DIR_DOWN || move_bag(bag, man_dir))
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
                        money_snd = 1; // Издать звук съедаемого золота
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
                man_x_graph = prev_man_x_graph;
                man_y_graph = prev_man_y_graph;
            }
            else
            {
                if (man_x_graph != prev_man_x_graph || man_y_graph != prev_man_y_graph) // Если Диггер переместился
                {
                     // Очистить биты фона, который был "прогрызен"
                    clear_background_bits(man_x_graph, man_y_graph, man_dir);
                    clear_background_bits(man_x_graph, man_y_graph, man_dir ^ 1);

                    // Стереть след от Диггера с нужной стороы
                    erase_trail(man_dir, man_x_graph, man_y_graph);

                    // Нарисовать "прогрыз" от движения Диггера
                    gnaw(man_dir, prev_man_x_graph, prev_man_y_graph);

                    // Удалить монеты съеденные Диггером
                    const uint8_t man_abs_x_pos = man_x_graph - FIELD_X_OFFSET;
                    const uint8_t man_abs_y_pos = man_y_graph - FIELD_Y_OFFSET;
                    const uint8_t man_x_log = man_abs_x_pos / POS_X_STEP;
                    const uint8_t man_y_log = man_abs_y_pos / POS_Y_STEP;

                    if (remove_coin(man_x_log, man_y_log))
                    {
                        eat_coin();
                    }
                }
            }

            draw_man(); // Нарисовать Диггера

            // Проверить Диггера на сопркосновение с врагами
            for (uint8_t i = 0; i < bugs_max; ++i)
            {
                struct bug_info *bug = &bugs[i]; // Структура с информацией о враге

                if (bug->state != CREATURE_ALIVE) continue; // Пропустить дохлых врагов

                // Если Диггер не касается врага
                if (!check_collision_4_15(bug->x_graph, bug->y_graph, man_x_graph, man_y_graph)) continue;

                if (bonus_state == BONUS_ON)
                {
                    // Если включен режим Бонус
                    bug_snd = 1; // Включить звук съедения врага
                    add_score(bonus_count * 200); // 200 * bonus_count очков за каждого съеденного врага
                    bonus_count <<= 1; // Удвоить bonus_count

                    // Стереть съеденного врага
                    erase_4_15(bug->x_graph, bug->y_graph);
                    bug->state = CREATURE_INACTIVE; // Деактивировать врага

                    bugs_active--; // Уменьшить количество активных врагов
                    bugs_total++;  // Увеличить количество создаваемых врагов компенсируя съеденных
                }
                else
                {
                    erase_4_15(man_x_graph, man_y_graph); // Стереть Диггера
                    man_rip();
                }
            }

            man_prev_dir = man_dir;
        }
    }
    else
    {
        // Перемещение и отрисовка убитого Диггера
        if (man_state == CREATURE_DEAD_MONEY_BAG)
        {
            uint8_t bag_y_pos = man_dead_bag->y_graph; // Вертикальная позиция мешка от которого погиб Диггер
            if (bag_y_pos > man_y_graph)
            {
                erase_trail(DIR_DOWN, man_x_graph, man_y_graph);
                man_y_graph = bag_y_pos; // Если мешок опустился ниже Диггера, Диггер перемещается за мешком
            }

            // Нарисовать перевёрнутого Диггера
            sp_put(man_x_graph, man_y_graph, sizeof(image_digger_turned_over[0]), sizeof(image_digger_turned_over) / sizeof(image_digger_turned_over[0]),
                   (uint8_t *)image_digger_turned_over, (uint8_t *)outline_digger_turned_over);

            if (man_dead_bag->dir == DIR_STOP)
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
void man_rip()
{
    // Последовательность высоты на которую подпрыгивает перевёрнутый Диггер
    static uint8_t bounce[8] = { 3, 5, 6, 6, 5, 4, 3, 0 };

    uint16_t prev_y_graph = 0;
    uint16_t period = 19000 / N;
    uint16_t i = 0;
    while (period < 36000 / N) // Звук убиения Диггера
    {
        if (snd_effects) sound(period, 2);

        if (man_state != CREATURE_DEAD_MONEY_BAG)
        {
            uint16_t y_graph = man_y_graph - bounce[i >> 3];
            if (prev_y_graph)
            {
                sp_put(man_x_graph, prev_y_graph, sizeof(outline_digger_turned_over[0]), sizeof(outline_digger_turned_over) / sizeof(outline_digger_turned_over[0]),
                    0, (uint8_t *)outline_digger_turned_over);
            }

            sp_put(man_x_graph, y_graph, sizeof(image_digger_turned_over[0]), sizeof(image_digger_turned_over) / sizeof(image_digger_turned_over[0]),
                (uint8_t *)image_digger_turned_over, (uint8_t *)outline_digger_turned_over);
            prev_y_graph = y_graph;
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

    // TODO: Удалить врагов с которыми коллизия
    // for (uint8_t i = 0; i < bugs_max; ++i)
    // {
    //     struct bug_info *bug = &bugs[i]; // Структура с информацией о враге
    //     if (!bugs_active) continue; // Пропустить неактивных врагов
    //
    //     // Проверить, что враг оказался рядом с могилкой
    //     if (check_collision_4_15(man_x_graph, man_y_graph, bug->x_graph, bug->y_graph))
    //     {
    //         bug->count = 1;
    //         bug->state = CREATURE_RIP; // Враг был убит выстрелом
    //     }
    // }

    // Траурный марш
    static const uint8_t music_dead_periods[]   = { C4 / NV, C4 / NV, C4 / NV, C4 / NV, DS4 / NV, D4 / NV, D4 / NV, C4 / NV, C4 / NV, B3 / NV, C4 / NV  };
    static const uint16_t music_dead_durations[] = { N6, NQ, NE, N6, NQ, NE, NQ, NE, NQ, NE, N12 };

    for (uint16_t i = 0; i < sizeof(music_dead_periods)/ sizeof(music_dead_periods[0]); ++i)
    {
        uint8_t period = music_dead_periods[i];
        uint16_t duration = music_dead_durations[i];
        if (snd_effects) sound_vibrato(period, duration);

        if (i < sizeof(image_rip) / sizeof(image_rip[0]))
        {
            sp_4_15_put(man_x_graph, man_y_graph, (uint8_t *)image_rip[i]);
        }

        delay_ms(30);
    }

    erase_4_15(man_x_graph, man_y_graph); // Стереть надгробный камень

    man_state = CREATURE_RIP;
}

void bonus_indicator(uint8_t color)
{
    sp_paint_brick(0, 0, SCREEN_BYTE_WIDTH, SCREEN_Y_OFFSET, color);
    sp_paint_brick(0, 255 - SCREEN_Y_OFFSET, SCREEN_BYTE_WIDTH, SCREEN_Y_OFFSET, color);
}

/**
 * @brief Обработка бонуса
 */
void process_bonus()
{
    // Обработка Бонус-режима
    if (bonus_state == BONUS_ON) // Если включен Бонус-режим
    {
        if ((man_state == CREATURE_ALIVE) && bonus_time) // Если Диггер жив и время Бонус-режима не закончилось
        {
            bonus_time--; // Декрементировать время Бонус-режима

            // Мигание в начале и в конце времени Бонус-режима
            if (bonus_flash || bonus_time < 20)
            {
                bonus_flash--;
                chase_snd = bonus_flash;

                // Мигание при включении бонус-режима
                bonus_indicator((bonus_time & 1) ? 0xFF : 0x00);

                // TODO: Включить музыку бонус-режима
            }
        }
        else
        {
            bonus_state = BONUS_END;
            chase_snd = 0; // Выключить звук включения/выключения бонус-режима
            bugs_delay_counter = bugs_delay; //  Перезарядить счётчик времени до появления врагов

            // TODO: Включить музыку Popcorn
        }
    }
}

/**
 * @brief Обработка общего состояния игры (переход на новый уровень, Game Over и т.д.)
 */
void process_game_state()
{
    // Декрементировать таймер между последовательными съедениями драгоценных камней (монеток)
    if (coin_time > 0) coin_time--;
    else coin_snd_note = -1;

    if (man_state == CREATURE_RIP)
    {
        lives--;                    // Уменьшить количество жизней
        print_lives();              // Вывести количество жизней

        if (lives > 0) // Проверить остались ли ещё жизни
        {
            // Если жизни остались
            init_level_state(); // Инициализировать состояние уровня
            man_state = CREATURE_ALIVE; // Оживить Диггера
        }
        else
        {
#if !defined(DEBUG)
            // constexpr uint16_t go_width = sizeof(game_over[0]);
            // constexpr uint16_t go_height = sizeof(game_over) / go_width;
            // constexpr uint16_t go_x = (SCREEN_BYTE_WIDTH - go_width) / 2;
            // constexpr uint16_t go_y = (SCREEN_PIX_HEIGHT - go_height) / 2;
            //
            // // Вывести надпись "Game Over"
            // sp_paint_brick(go_x - 2 * MOVE_X_STEP, go_y - 2 * MOVE_Y_STEP, go_width + 4 * MOVE_X_STEP, go_height + 4 * MOVE_Y_STEP, 0xFF);
            // sp_paint_brick(go_x - MOVE_X_STEP, go_y - MOVE_Y_STEP, go_width + 2 * MOVE_X_STEP, go_height + 2 * MOVE_Y_STEP, 0);
            // sp_put(go_x, go_y, go_width, go_height, (uint8_t *)game_over, 0);
            // delay_ms(5000);
#endif
            init_game(); // Установить игру в начальное состояние
        }
    }

    if (done_snd)
    {
        done_snd = 0;

        // Циклическое увеличение номера уровня
        level_no++;
        level_no &= LEVELS_NUM - 1;

        // Увеличение сложности после прохождения очередного уровня (максимальный уровень 9)
        if (difficulty < 10) difficulty++;

        init_level(); // Инициализация нового уровня
    }
}

extern void start();

/**
 * @brief Основная программа
 */
void main()
{
    // EMT_14();
    //
    typedef void (*vector)();
    *((volatile vector *)VEC_STOP) = start; // Установить вектор клавиши "СТОП" на _start

    // EMT_16(0233);
    // EMT_16(0236);

    set_PSW(1 << PSW_I); // Замаскировать прерывания IRQ
    ((union KEY_STATE *)REG_KEY_STATE)->bits.INT_MASK = 1; // Отключить прерывание от клавиатуры

    sp_paint_brick(0, 0, SCREEN_BYTE_WIDTH, FIELD_Y_OFFSET - MOVE_Y_STEP * 3, 0); // Очистить экран с верха до начала игрового поля

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

        const uint8_t man_abs_x_pos = man_x_graph - FIELD_X_OFFSET;
        const uint8_t man_abs_y_pos = man_y_graph - FIELD_Y_OFFSET;
        const uint8_t man_x_log = man_abs_x_pos / POS_X_STEP;
        const uint8_t man_y_log = man_abs_y_pos / POS_Y_STEP;
        const uint8_t man_x_rem = man_abs_x_pos % POS_X_STEP;
        const uint8_t man_y_rem = (man_abs_y_pos % POS_Y_STEP) >> 2;

        process_bugs();
        process_bags(man_x_log, man_y_log);
        process_missile();
        process_man(man_x_rem, man_y_rem);
        process_bonus();
        if (snd_effects) sound_effect();
        process_game_state();

#if defined(DEBUG)
        draw_coin_minimap(); // Нарисовать мини-карту монеток
        draw_bg_minimap();   // Нарисовать мини-карту ячеек фона
        // Рспечатать оставшееся свободное время
        print_dec(*((volatile uint16_t *)REG_TVE_COUNT), 0, MAX_Y_POS + 2 * POS_Y_STEP);
#endif
        sound_vibrato(music_popcorn[music_idx][0] * 4, *((volatile uint16_t *)REG_TVE_COUNT) / 16);
        if (++music_count >= 1 << music_popcorn[music_idx][1])
        {
            music_count = 0;
            if (++music_idx >= sizeof(music_popcorn) / sizeof(music_popcorn)[0]) music_idx = 0;
        }

        while ((tve_csr->reg & (1 << TVE_CSR_FL)) == 0); // Ожидать срабатывания таймера.
    }
}
