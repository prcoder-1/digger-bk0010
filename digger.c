#include "memory.h"
#include "sprites.h"
#include "sound.h"
#include "tools.h"
#include "emt.h"
#include "digger_sprites.h"
#include "digger_font.h"
#include "digger_levels.h"
#include "digger_music.h"

#define DEBUG // Режим отладки включен

#define FIELD_X_OFFSET 2  // Смещение игрового поля по оси X
#define FIELD_Y_OFFSET 32 // Смещение игрового поля по оси Y
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
#define BONUS_IND_START (MAX_Y_POS + 28)
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
    CREATURE_ALIVE,          /**< Жив */
    CREATURE_STARTING,       /**< Стратует */
    CREATURE_DEAD_MONEY_BAG, /**< Убит мешком с деньгами */
    CREATURE_RIP,            /**< Лежит дохлый */
    CREATURE_AFTER_RIP       /**< После того, как появился RIP */
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
    struct bag_info *dead_bag; ///< Указатель на мешок от котрого погиб враг
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
uint16_t man_image_phase;      // Фаза отображения спрайта Диггера
uint16_t man_image_phase_inc;  // Инкремент(декремент) фазы отображения спрайта Диггера
uint16_t man_wait;             // Задержка перед следующим перемещением Диггера
uint16_t man_x_graph;          // Положение Диггера по оси X в графических координатах
uint16_t man_y_graph;          // Положение Диггера по оси Y в графических координатах
enum direction man_dir;        // Направление движения Диггера
enum direction man_new_dir;    // Желаемое направление движение Диггера (команда с клавиатуры)
enum direction man_prev_dir;   // Предыдущее направление движения Диггера
enum creature_state man_state; // Состояние Диггера (жив, убит, лежит дохлый)
struct bag_info *man_dead_bag; // Указатель на мешок от котрого погиб Диггер

// Переменные отвечающие за создание врагов
uint8_t bugs_max;           // Максимальное количество врагов на уровне одновременно
uint8_t bugs_total;         // Общее количество врагов на уровне
uint8_t bugs_delay;         // Задержка перед рождением врага (Ноббина)
uint8_t bugs_delay_counter; // Счётчик задержки перед рождением врага
uint8_t bugs_active;        // Количество активных врагов
uint8_t bugs_created;       // Общее количество сщзданных врагов

// Переменные отвечающие за мешки
uint8_t broke_max; // Время через которое исчезнет разбившийся мешок

// Переменные отвечающие за бонус-режим
enum bonus_state bonus_state; // Состояние режима бонус
uint16_t bonus_time;          // Время активности бонус-режима
uint8_t  bonus_flash;         // Время мерцания при включении/выключении Бонус-режима
uint8_t  bonus_count;         // Множитель очков в Бонус-режиме (умножается на два за каждого пойманного врага)
uint32_t bonus_life_score;    // Количество очков для дополнительное жизни

// Переменные отвечающие за выстрел
// uint8_t mis_ready;   // Флаг готовности снаряда к выстрелу
// uint8_t mis_wait;    // Задержка готовности выстрела
// uint8_t mis_explode; // Счётчик взрывающегося снаряда

// Переменные отвечающие за состояние игры
uint16_t difficulty; // Уровень сложности
uint16_t level_no;   // Текущий номер уровня
int16_t  lives;      // Количество жизней
uint32_t score;      // Количество очков

// Переменные отвечающие за вывод звуков
uint8_t  man_rip_snd;     // Флаг, означающий, что звук и анимация RIP включены
uint8_t  chase_snd;       // Флаг, означающий, что звук включения бонус-режима активн
uint8_t  chase_snd_flip;  // Флаг переключающий тональность звука при включении/выключении бонус-режима
uint8_t  loose_snd;       // Флаг, означающий, что звук качающегося мешка включен
uint16_t loose_snd_phase; // Фаза звука качающегося мешка
uint8_t  fall_snd;        // Флаг, означающий, что звук летящего мешка включен
uint8_t  fall_snd_phase;  // Фаза звука падающего мешка
uint16_t fall_period;     // Период звука летящего мешка
uint8_t  break_bag_snd;   // Флаг, означающий, что звук разбивающегося мешка включен
uint8_t  money_snd;       // Флаг, означающий, что звук съедания золота включен
uint8_t  coin_snd;        // Флаг, означающий, что звук съедания монетки включен
int8_t   coin_snd_note;   // Номер ноты при съедании монетки (драгоценного камня)
uint8_t  coin_time;       // Таймер между последовательными съедениями драгоценных камней (монеток)
uint8_t  done_snd;        // Флаг, означающий, что звук завершения уровня включен
uint8_t  bug_snd;         // Флаг, означающий, что звук съедания врага в бонус-режиме включен
uint8_t  life_snd;        // Флаг, означающий, что звук получения дополнительной жизни включен

#if defined(DEBUG)
/**
 * @brief Отладочная процедура отображения мини-карты состояния фона
 */
void draw_bg_minimap()
{
    sp_put(48, MOVE_Y_STEP + 2, sizeof(background[0]), sizeof(background) / sizeof(background[0]), (uint8_t*)background, 0);
}

/**
 * @brief Отладочная процедура отображения мини-карты состояния монеток (драгоценных камней)
 */
void draw_coin_minimap()
{
    sp_put(45, MOVE_Y_STEP + 2, sizeof(coins[0]), sizeof(coins) / sizeof(coins[0]), (uint8_t*)coins, 0);
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
    char buf[5]; // Буфер для 5 десятичных знаков

    for (int i = 0; i < sizeof(buf); ++i) buf[i] = '0';

    char *ptr = &buf[sizeof(buf)];
    uint_to_str(number, &ptr);

    for (uint8_t i = 0; i < sizeof(buf); ++i)
    {
        uint16_t index = buf[i] - '0';
        sp_put(x_graph, y_graph, sizeof(ch_digits[0][0]), sizeof(ch_digits[0]) / sizeof(ch_digits[0][0]), (uint8_t *)ch_digits[index], nullptr); // Вывести спрайт цифры
        x_graph += sizeof(ch_digits[0][0]);
    }
}

/**
 * @brief Вывод количества жизней в виде спрайтов Диггера рядом с количеством очков
 */
void print_lives()
{
    constexpr uint16_t man_x_offset = sizeof(ch_digits[0][0]) * 5 + 1; // Смещение шириной в пять символов '0' плюс один байт (4 пикселя)
    constexpr uint16_t man_y_offset = 2;
    constexpr uint16_t one_pos_width = sizeof(image_digger_right[1][0]) + 1;
    constexpr uint16_t width = MAX_LIVES * one_pos_width;
    constexpr uint16_t height = sizeof(image_digger_right[1]) / sizeof(image_digger_right[1][0]);

    sp_paint_brick(man_x_offset, man_y_offset, width, height, 0);

    uint16_t l = 1;
    for (uint16_t i = man_x_offset; i < man_x_offset + width; i += one_pos_width)
    {
        if (++l > lives) break;
        sp_4_15_put(i, man_y_offset, (uint8_t *)image_digger_right[1]);
    }
}

/**
 * @brief Добавление заданного количества очков и аечать очков в левом верхнем углу экрана
 */
void add_score(uint16_t score)
{
    score += score;
    print_dec(score, 0, MOVE_Y_STEP);

     // Если количество жизней не достигло максимального и клоичество очков досигло бонусного для получения жизни
    if (lives < MAX_LIVES && (score >= bonus_life_score))
    {
        lives++; // Увеличичить количество жизней на единицу
        print_lives(); // Вывесли количество жизней
        bonus_life_score += BONUS_LIFE_SCORE; // Количество очков до следующего бонуса в виде жизни

        life_snd = 24; // Издать звук получения жизни
    }
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
int check_collision(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dist_x, uint8_t dist_y)
{
    return (ABS(x2 - x1) < dist_x) && (ABS(y2 - y1) < dist_y);
}

/**
 * @brief Стирание фона в правую сторону
 */
void era_right(uint16_t x, uint16_t y)
{
    sp_put(x + 4, y - 1, sizeof(outline_blank_right[0]), sizeof(outline_blank_right)/sizeof(outline_blank_right[0]), nullptr, (uint8_t*)outline_blank_right);
}

/**
 * @brief Стирание фона в левую сторону
 */
void era_left(uint16_t x, uint16_t y)
{
    sp_put(x - 2, y - 1, sizeof(outline_blank_left[0]), sizeof(outline_blank_left)/sizeof(outline_blank_left[0]), nullptr, (uint8_t*)outline_blank_left);
}

/**
 * @brief Стирание фона вверх
 */
void era_up(uint16_t x, uint16_t y)
{
    sp_put(x - 1, y - 7, sizeof(outline_blank_up[0]), sizeof(outline_blank_up)/sizeof(outline_blank_up[0]), nullptr, (uint8_t*)outline_blank_up);
}

/**
 * @brief Стирание фона вниз
 */
void era_down(uint16_t x, uint16_t y)
{
    sp_put(x - 1, y + 15, sizeof(outline_blamk_down[0]), sizeof(outline_blamk_down)/sizeof(outline_blamk_down[0]), nullptr, (uint8_t*)outline_blamk_down);
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

/**
 * @brief Инициализация переменных состояния перед старом уровня
 */
void init_level_state()
{
    bonus_state = BONUS_OFF;

    // Отключение индикации бонус-режима
    sp_paint_brick(0, BONUS_IND_START, SCREEN_BYTE_WIDTH, SCREEN_PIX_HEIGHT - BONUS_IND_START, 0);

    // Деактивировать всех врагов
    for (uint8_t i = 0; i < MAX_BUGS; ++i)
    {
        struct bug_info *bug = &bugs[i]; // Структура с информацией о враге
        if (bug->state == CREATURE_INACTIVE) continue; // Пропустить неактивных врагов

        erase_4_15(bug->x_graph, bug->y_graph); // Стереть врага
        bug->state = CREATURE_INACTIVE; // Деактивировать врага
    }

    if (difficulty >= 9) bugs_max = 5;      // На уровне сложности 9 и выше максимально 5 врагов одновременно
    else if (difficulty >= 6) bugs_max = 4; // На уровне сложности с 6 до 8 *включительно) до 4 врагов одновременно
    else bugs_max = 3;                      // На уровнях до шестого - максимально три варага одновременно

    // Переменные относщиеся к созданию и управлению врагами
    bugs_total = difficulty + 5;         // Общее количество врагов на уровне - пять плюс уровень сложности
    bugs_delay = 45 - (difficulty << 1); // Задержка появления врагов (с ростом сложности убывает)
    bugs_delay_counter = bugs_delay;     // Инициализация счётчика задержки врага исходным значением
    bugs_active = 0;                     // Количество активных врагов
    bugs_created = 0;                    // Общее количество сщзданных врагов

    broke_max = 150 - difficulty * 10; // Время через которое исчезнет разбившийся мешок (с ростом сложности убывает)

    // Инициализация переменных Диггера
    man_dir = DIR_RIGHT;
    man_new_dir = DIR_RIGHT;
    man_prev_dir = DIR_RIGHT;
    man_x_graph = FIELD_X_OFFSET + MAN_START_X * POS_X_STEP; // Исходная координата Диггера на экране по оси X
    man_y_graph = FIELD_Y_OFFSET + MAN_START_Y * POS_Y_STEP; // Исходная координата Диггера на экране по оси Y
    man_image_phase = 0;        // Фаза анимации Диггера
    man_image_phase_inc = 1;    // Направление ихменения фазы анимации Диггера
    man_wait = 0;               // Задержка перед следующим перемещением Диггера
    man_state = CREATURE_ALIVE; // Исходное состояние - Диггер жив

    // mis_ready = 1;
    // mis_wait = 0;
    // mis_explode = 0;

    // Инициализация переменных используемых для звуковых эффектов
    coin_snd = 0;
    coin_snd_note = -1;
    coin_time = 0;
    loose_snd = 0;
    fall_snd = 0;
    money_snd = 0;
    man_rip_snd = 0;
    chase_snd = 0;
    chase_snd_flip = 0;
    done_snd = 0;
    bug_snd = 0;
    life_snd = 0;
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
        uint16_t x_graph;
        for (x_graph = 0; x_graph < x_size * bg_block_width; x_graph += bg_block_width)
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
            *bg = 0xFF; // устанавливаем все биты h_bite и v_bite в единицу (вся клетка фона цела)

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
                *bg &= 0xF0;  // сбрасываем все биты h_bite фона для горизонтальных проходов
                for (uint16_t i = 4; i > 0; --i)
                {
                    era_right(x_graph - i, y_graph);
                }
                era_left (x_graph + 1, y_graph);
            }

            if (ls == LEV_V || ls == LEV_S)
            {
                *bg &= 0x0F;  // сбрасываем все биты v_bite фона для вертикальных проходов
                for (uint16_t i = 15; i > 0; i -= 3)
                {
                    era_down(x_graph, y_graph - i);
                }
                era_up  (x_graph, y_graph + 3);
            }

            x_graph += POS_X_STEP;
        }

        x_graph = FIELD_X_OFFSET;
        y_graph += POS_Y_STEP;
    }

    draw_coin_minimap();
    draw_bg_minimap();
    init_level_state();
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
    if (!(byte & 0xF0) || !(byte & 0xF)) return 1;

    return 0;
}

/**
 * @brief Определяет возможность движения врага в заданном направлении
 *
 * @param dir - направлкние движения
 * @param bug - структура с информацией о враге
 *
 * @return - 1 - движение в заданном направлении возможно, 0 - движение в заданном направлении невозможно
 */
uint8_t check_path(enum direction dir, struct bug_info *bug)
{
    uint8_t abs_x_pos = bug->x_graph - FIELD_X_OFFSET;
    uint8_t abs_y_pos = bug->y_graph - FIELD_Y_OFFSET;
    uint8_t x_log = abs_x_pos / POS_X_STEP;
    uint8_t y_log = abs_y_pos / POS_Y_STEP;

    switch (dir)
    {
        case DIR_RIGHT:
        {
            if (x_log >= (W_MAX - 1)) return 0; // Если находимся у правого края
            // Если клетка правее полностью проедена, а также есть прокус слева в клетке правее или прокус справа в текущей клетке
            if (full_bite(background[y_log][x_log + 1]) && (((background[y_log][x_log + 1] & 1) == 0) || ((background[y_log][x_log] & 8) == 0))) return 1;
            break;
        }

        case DIR_LEFT:
        {
            if (!x_log) return 0; // Если находимся у левого края
            // Если клетка левее полность проедена, и есть в ней прокус справа или прокус в текущей клетке слева клетки
            if (full_bite(background[y_log][x_log - 1]) && (((background[y_log][x_log - 1] & 8) == 0) || ((background[y_log][x_log] & 1) == 0))) return 1;
            break;
        }

        case DIR_DOWN:
        {
            if (y_log >= (H_MAX - 1)) return 0; // Если находимся у нижнего края
            // Если клетка ниже полность проедена, и есть в ней прокус сверху или снизу текущей клетки
            if (full_bite(background[y_log + 1][x_log]) && (((background[y_log + 1][x_log] & 0x10) == 0) || ((background[y_log][x_log] & 0x80) == 0))) return 1;
            break;
        }

        case DIR_UP:
        {
            if (!y_log) return 0; // Если находимся у верхнего края
            // Если клетка выше полность проедена, и есть в ней прокус снизу или сверху текущеёклетки
            if (full_bite(background[y_log - 1][x_log]) && (((background[y_log - 1][x_log] & 0x80) == 0) || ((background[y_log][x_log] & 0x10) == 0))) return 1;
            break;
        }
    }

    return 0;
}

/**
 * @brief Определяет возможность движения врага в заданном направлении
 *
 * @param dir - направлкние движения
 * @param bug - структура с информацией о враге
 *
 * @return - 1 - движение в заданном направлении возможно, 0 - движение в заданном направлении невозможно
 */
void era_background(uint16_t x_graph, uint16_t y_graph, enum direction dir)
{
    uint16_t abs_x_pos = x_graph - FIELD_X_OFFSET;
    uint16_t abs_y_pos = y_graph - FIELD_Y_OFFSET;
    uint16_t x_log = abs_x_pos / POS_X_STEP;
    uint16_t y_log = abs_y_pos / POS_Y_STEP;
    int16_t x_rem = abs_x_pos % POS_X_STEP;
    int16_t y_rem = (abs_y_pos % POS_Y_STEP) >> 2;

    switch (dir)
    {
        case DIR_LEFT:
        {
            x_rem--;

            if (x_rem < 0)
            {
                x_rem += 4;
                x_log--;
            }

            break;
        }

        case DIR_RIGHT:
        {
            x_log++; // Следующая позиция матрицы по X

            break;
        }

        case DIR_UP:
        {
            y_rem--;

            if (y_rem < 0)
            {
                y_rem += 4;
                y_log--;
            }

            break;
        }

        case DIR_DOWN:
        {
            y_log++; // Следущая позиция матрицы по Y

            break;
        }
    }

    switch (dir)
    {
        case DIR_LEFT:
        case DIR_RIGHT:
        {
            // Стереть соответсвующий бит матрицы фона
            background[y_log][x_log] &= ~(1 << x_rem);
            break;
        }

        case DIR_UP:
        case DIR_DOWN:
        {
            // Стереть соответсвующий бит матрицы фона
            background[y_log][x_log] &= ~(1 << (y_rem + 4));
            break;
        }
    }

    draw_bg_minimap();
}

/**
 * @brief Проверка на выход за пределы игрового поля
 */
int check_out_of_range(enum direction dir, uint16_t x_graph, uint16_t y_graph)
{
    // print_dec(x_graph, 0, MAX_Y_POS + 2 * POS_Y_STEP);
    // print_dec(y_graph, sizeof(ch_digits[0][0]) * 5 + 1, MAX_Y_POS + 2 * POS_Y_STEP);

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
        if (check_collision(bag_x_graph, bag_y_graph, man_x_graph, man_y_graph, 4, 15))
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
            if (check_collision(bag_x_graph, bag_y_graph, bug->x_graph, bug->y_graph, 4, 15))
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
            if (check_collision(bag_x_graph, bag_y_graph, another_bag->x_graph, another_bag->y_graph, 4, 15))
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

    if (rv)
    {
        // Мешок небыл перемещён
        break_bag_snd = 1; // Издать звук разбившегося мешка
    }
    else
    {
        uint8_t bag_abs_x_pos = bag_x_graph - FIELD_X_OFFSET;
        uint8_t bag_x_rem = bag_abs_x_pos % POS_X_STEP;

        // Стирание мешка по старым координатам
        sp_put(bag->x_graph, bag->y_graph, 4, 15, nullptr, (uint8_t *)outline_bag);

        // Отрисовка спрайта передвигаемого мешка
        sp_put(bag_x_graph, bag_y_graph, 4, 15, (uint8_t *)image_bag, (uint8_t *)outline_bag);

        // Установить новые координаты мешка
        bag->dir = dir;
        bag->x_graph = bag_x_graph;
        bag->y_graph = bag_y_graph;
    }

    return rv;
}

/**
 * @brief Обработка перемещения Ноббина/Хоббина
 *
 * @param bug - указатель на структуру с информацией о враге
 */
void move_bug(struct bug_info *bug)
{
    enum direction dir_1, dir_2, dir_3, dir_4;

    uint8_t bug_x_graph = bug->x_graph;
    uint8_t bug_y_graph = bug->y_graph;
    uint8_t bug_abs_x_pos = bug_x_graph - FIELD_X_OFFSET;
    uint8_t bug_abs_y_pos = bug_y_graph - FIELD_Y_OFFSET;
    uint8_t bug_x_rem = bug_abs_x_pos % POS_X_STEP;
    uint8_t bug_y_rem = (bug_abs_y_pos % POS_Y_STEP) >> 2;

    // Проверка возможности изменения направления движения при нахождении на ровной границе клетки
    if (!bug_x_rem && !bug_y_rem)
    {
        // Если Хоббин застрял на время более заданного, то превратить его в Ноббина
        if ((bug->type == BUG_HOBBIN) && (bug->count > (30 + difficulty * 2)))
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
        if ((difficulty < 6) && ((rand() & 0xF) > (difficulty + 10)))
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
                 if (check_path(dir_1, bug)) { dir = dir_1; }
            else if (check_path(dir_2, bug)) { dir = dir_2; }
            else if (check_path(dir_3, bug)) { dir = dir_3; }
            else if (check_path(dir_4, bug)) { dir = dir_4; }
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

    // Остановить врага при попытке выхода за пределы экрана
    if (check_out_of_range(bug->dir, bug_x_graph, bug_y_graph))
    {
        bug->dir = DIR_STOP;
    }

    // Для Хоббинов прокапывающих новый туннель
    if (bug->type == BUG_HOBBIN)
    {
        // Сделать"прокус" в массиве бит фона
        era_background(bug_x_graph, bug_y_graph, bug->dir);

        // Стерерь кусочек фона на экране в соответствии с направлением движения и текущим положением
        switch (bug->dir)
        {
            case DIR_RIGHT: { era_right(bug_x_graph, bug_y_graph); break; }
            case DIR_LEFT:  { era_left (bug_x_graph, bug_y_graph); break; }
            case DIR_DOWN:  { era_down (bug_x_graph, bug_y_graph); break; }
            case DIR_UP:    { era_up   (bug_x_graph, bug_y_graph); break; }
        }

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
            switch (bug->dir)
            {
                case DIR_RIGHT: { bug->x_graph += MOVE_X_STEP; break; }
                case DIR_LEFT:  { bug->x_graph -= MOVE_X_STEP; break; }
                case DIR_DOWN:  { bug->y_graph += MOVE_Y_STEP; break; }
                case DIR_UP:    { bug->y_graph -= MOVE_Y_STEP; break; }
            }
        }
    }

    if (bug->state == CREATURE_ALIVE)
    {
        // Проверить соприкосновение врага с мешками
        for (uint8_t i = 0; i < MAX_BAGS; ++i)
        {
            struct bag_info *bag = &bags[i]; // Структура с информацией о мешках

            if (bag->state == BAG_INACTIVE) continue; // Пропустить неактивные мешки

            if (check_collision(bag->x_graph, bag->y_graph, bug_x_graph, bug_y_graph, 4, 15))
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

                    if (bag->state == BAG_BROKEN) // Если мешок разбит
                    {
                        remove_bag = 1; // Удалить съеденное Ноббином золото
                        bug->wait++; // Задержать врага съедающего золото
                    }
                    else
                    {
                        // Если мешок не разбит
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

    // Подтереть след врага с нужной стороны
    switch (bug->dir)
    {
        case DIR_LEFT:
        {
            sp_paint_brick(bug_x_graph + 4 - MOVE_X_STEP, bug_y_graph, MOVE_X_STEP, 15, 0);
            break;
        }

        case DIR_RIGHT:
        {
            sp_paint_brick(bug_x_graph, bug_y_graph, MOVE_X_STEP, 15, 0);
            break;
        }

        case DIR_UP:
        {
            sp_paint_brick(bug_x_graph, bug_y_graph + 15 - MOVE_Y_STEP, 4, MOVE_Y_STEP, 0);
            break;
        }

        case DIR_DOWN:
        {
            sp_paint_brick(bug_x_graph, bug_y_graph, 4, MOVE_Y_STEP, 0);
            break;
        }
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
            sp_4_15_put(bug->x_graph, bug->y_graph, (uint8_t *)image_hobbin_right[bug->image_phase]);
        }
        else
        {
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
    bag->state = (bag->count > 1) ? BAG_BREAKS : BAG_STATIONARY; // То он будет разбиваться
    bag->dir = DIR_STOP; // Остановить мешок
    bag->count = 0;

    // TODO: Унчтожить все мешки под тем, который остановился
}

/**
 * @brief Подпрограмма отрисовки Диггера
 */
void draw_man()
{
    uint8_t cab = 1;
    // uint8_t cab = mis_ready && !mis_wait; // Флаг наличия "башенки"

    man_image_phase += man_image_phase_inc; // Переключить фазу изображения

    // При необходимости, сменить направление изменения фазы
    if (!man_image_phase || man_image_phase >= 2) man_image_phase_inc =- man_image_phase_inc;

    const uint8_t *image = 0;

    switch (man_dir)
    {
        case DIR_RIGHT:
        case DIR_LEFT:
        {
            image = (uint8_t *)image_digger_right[man_image_phase + ((cab) ? 0 : 3)];
            if (man_dir == DIR_RIGHT)
            {
                sp_4_15_put(man_x_graph, man_y_graph, image);
            }
            else
            {
                sp_4_15_h_mirror_put(man_x_graph, man_y_graph, image);
            }
            break;
        }

        case DIR_UP:
        case DIR_DOWN:
        {
            image = (uint8_t *)image_digger_up[man_image_phase + ((cab) ? 0 : 3)];
            if (man_dir == DIR_UP)
            {
                sp_4_15_put(man_x_graph, man_y_graph, image);
            }
            else
            {
                sp_4_15_hv_mirror_put(man_x_graph, man_y_graph, image);
            }
            break;
        }

        case DIR_STOP:
        {
            break;
        }
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
    uint16_t coin_mask = 1 << x_log;
    if (coins[y_log] & coin_mask)
    {
        coins[y_log] &= ~coin_mask; // Сбросить бит соответствующий съеденной монете

        // Стереть съеденную монету (драгоценный камешек)
        sp_put(FIELD_X_OFFSET + x_log * POS_X_STEP, FIELD_Y_OFFSET + y_log * POS_Y_STEP + COIN_Y_OFFSET,
               sizeof(outline_no_coin[0]), sizeof(outline_no_coin) / sizeof(outline_no_coin[0]), nullptr, (uint8_t *)outline_no_coin);

        draw_coin_minimap();

        uint16_t level_done = 1;
        for (uint16_t i = 0; i < sizeof(coins) / sizeof(coins[0]); ++i)
        {
            if (coins[i]) { level_done = 0; break; }
        }

        if (level_done) done_snd = 1;

        return 1;
    }

    return 0;
}

/**
 * @brief Подпрограмма перемещения Диггера
 */
void move_man()
{
    uint8_t man_abs_x_pos = man_x_graph - FIELD_X_OFFSET;
    uint8_t man_abs_y_pos = man_y_graph - FIELD_Y_OFFSET;
    uint8_t man_x_log = man_abs_x_pos / POS_X_STEP;
    uint8_t man_y_log = man_abs_y_pos / POS_Y_STEP;
    uint8_t man_x_rem = man_abs_x_pos % POS_X_STEP;
    uint8_t man_y_rem = (man_abs_y_pos % POS_Y_STEP) >> 2;

    // Обработка управления с клавиатуры
    if (((union KEY_STATE *)REG_KEY_STATE)->reg & (1 << KEY_STATE_STATE)) // Если поступил новый скан-код клавиши
    {
        // Сохранить новое направление соответствующее полученному скан-коду
        uint8_t code = *((uint8_t *)REG_KEY_DATA);
        switch (code)
        {
            case 8:  man_new_dir = DIR_LEFT;  break; // Стрелка влево
            case 25: man_new_dir = DIR_RIGHT; break; // Стрелка вправо
            case 26: man_new_dir = DIR_UP;    break; // Стрелка вверх
            case 27: man_new_dir = DIR_DOWN;  break; // Стрелка вниз
#if defined(DEBUG)
            case 'D': difficulty++; break;            // Добавление уровня сложности
            case 'L': lives++; print_lives();  break; // Добавление жизни
            case 'N': done_snd = 1; break;            // Переход на следующий уровень
#endif
            case ' ': while (!(((union KEY_STATE *)REG_KEY_STATE)->reg & (1 << KEY_STATE_STATE))); break; // Пауза
            default: man_new_dir = DIR_STOP;
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

    // Если клавишу перестали удерживать, то остановиться
    if ((((union EXT_DEV *)REG_EXT_DEV)->bits.MAG_KEY)) man_dir = DIR_STOP;

    // Остановиться при попытке выхода за игровое поле
    if (check_out_of_range(man_dir, man_x_graph, man_y_graph))
    {
        man_dir = DIR_STOP;
    }

    if (bonus_state == BONUS_READY)
    {
        // Проверить что Диггер соприкоснулся с вишенкой
        if (check_collision(man_x_graph, man_y_graph, FIELD_X_OFFSET + (W_MAX - 1) * POS_X_STEP, FIELD_Y_OFFSET, 4, 15))
        {
            bonus_state = BONUS_ON;
            bonus_count = 1;
            bonus_time = 250 - difficulty * 20;
            bonus_flash = 19;

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

    if (man_dir != DIR_STOP) era_background(man_x_graph, man_y_graph, man_dir); /* update background matrix */

    uint8_t prev_man_x_graph = man_x_graph;
    uint8_t prev_man_y_graph = man_y_graph;

    switch (man_dir)
    {
        case DIR_RIGHT: { man_x_graph += MOVE_X_STEP; break; }
        case DIR_LEFT:  { man_x_graph -= MOVE_X_STEP; break; }
        case DIR_DOWN:  { man_y_graph += MOVE_Y_STEP; break; }
        case DIR_UP:    { man_y_graph -= MOVE_Y_STEP; break; }
    }

    if (man_dir == DIR_STOP) man_dir = man_prev_dir;

    if (remove_coin(man_x_log, man_y_log))
    {
        coin_snd = 7;  // Включить звук съедения монеты
        coin_time = 9; // Взвести таймер до последующего съедения монеты
        add_score(25); // 25 очков за съеденную монету (камешек)
        if (++coin_snd_note == 7) // Перейти к следующей ноте
        {
            coin_snd_note = -1;
            add_score(250); // 250 очков за съедение восьми последовательных монет
        }
    }

    uint16_t collision_flag = 0;

    // Обработка толкания мешков и съедения золота
    for (uint8_t i = 0; i < MAX_BAGS; ++i)
    {
        struct bag_info *bag = &bags[i]; // Структура с информацией о мешке
        if (bag->state == BAG_INACTIVE) continue; // Пропустить неактивные мешки

        // Если Диггер не соприкоснулся с мешком, проверить следующий мешок
        if (!check_collision(bag->x_graph, bag->y_graph, man_x_graph, man_y_graph, 4, 15)) continue;

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

            // case BAG_BREAKS:
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
            // Стереть след от Диггера с нужной стороы и нарисовать "прогрыз" со стороны в которую он движется
            switch (man_dir)
            {
                case DIR_RIGHT:
                {
                    sp_paint_brick(man_x_graph - MOVE_X_STEP, man_y_graph, MOVE_X_STEP, 15, 0);
                    era_right(prev_man_x_graph, prev_man_y_graph);
                    break;
                }

                case DIR_LEFT:
                {
                    sp_paint_brick(man_x_graph + 4, man_y_graph, MOVE_X_STEP, 15, 0);
                    era_left (prev_man_x_graph, prev_man_y_graph);
                    break;
                }

                case DIR_DOWN:
                {
                    sp_paint_brick(man_x_graph, man_y_graph - MOVE_Y_STEP, 4, MOVE_Y_STEP, 0);
                    era_down (prev_man_x_graph, prev_man_y_graph + 2);
                    break;
                }

                case DIR_UP:
                {
                    sp_paint_brick(man_x_graph, man_y_graph + 15, 4, MOVE_Y_STEP, 0);
                    era_up (prev_man_x_graph, prev_man_y_graph - 2);
                    break;
                }
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
        if (!check_collision(bug->x_graph, bug->y_graph, man_x_graph, man_y_graph, 4, 15)) continue;

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
            man_state = CREATURE_RIP;
            man_rip_snd = 1;
        }
    }

    man_prev_dir = man_dir;
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

    if (man_rip_snd) // Звук гибели Диггера плюс анимация подпрыгивания, надгробный камень и траурный марш
    {
        // Звук убиения Диггера

        // Последовательность высоты на которую подпрыгивает перевёрнутый Диггер
        static uint8_t bounce[8] = { 3, 5, 6, 6, 5, 4, 3, 0 };

        uint16_t period = 19000 / N;
        uint16_t i = 0;
        while (period < 36000 / N)
        {
            sound(period, 2);

            uint16_t y_graph = man_y_graph - bounce[i >> 3];
            era_up(man_x_graph, y_graph);
            // Анимация подпрыгивающего перевёрнутого Диггера
            sp_4_15_put(man_x_graph, y_graph, (uint8_t *)image_digger_turned_over);

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

        // Траурный марш
        #define NV 4
        static const uint16_t music_dead_periods[]   = { C4 / NV, C4 / NV, C4 / NV, C4 / NV, DS4 / NV, D4 / NV, D4 / NV, C4 / NV, C4 / NV, B3 / NV, C4 / NV  };
        static const uint16_t music_dead_durations[] = { N6, NQ, NE, N6, NQ,  NE, NQ, NE, NQ, NE, N12 };

        for (uint16_t i = 0; i < sizeof(music_dead_periods)/ sizeof(music_dead_periods[0]); ++i)
        {
            uint16_t period = music_dead_periods[i];
            uint16_t duration = music_dead_durations[i];
            sound_vibrato(period, duration);

            if (i < sizeof(image_rip) / sizeof(image_rip[0]))
            {
                sp_4_15_put(man_x_graph, man_y_graph, (uint8_t *)image_rip[i]);
            }

            delay_ms(30);
        }

        man_state = CREATURE_AFTER_RIP;
        man_rip_snd = 0;
    }

    if (chase_snd) // Звук включения бонус-режима
    {
        uint16_t durance = 100;
        chase_snd_flip = ~chase_snd_flip;
        if (chase_snd_flip) sound(1230 / N, durance);
        else sound(1513 / N, durance);
    }

    if (done_snd) // Звук завершения уровня
    {
        static const uint16_t done_periods[] = { C5 / NV, E5 / NV, G5 / NV, D5 / NV, F5 / NV, A5 / NV, E5 / NV, G5 / NV, B5 / NV, C5 / 2 / NV};

        for (uint16_t i = 0; i < sizeof(done_periods) / sizeof(done_periods[0]); ++i)
        {
            uint16_t period = done_periods[i];
            uint16_t durance = (i == 9) ? 800 : 300;
            sound_vibrato(period, durance);
            delay_ms(2);
        }
    }

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

    init_level();  // Начальная инициализация уровня
    add_score(0);  // Печать начального количества очков (нули)
    print_lives(); // Вывод начального количества жизней
}

/**
 * @brief Основная программа
 */
void main()
{
    // EMT_14();
    // EMT_16(0233);
    // EMT_16(0236);

    set_PSW(1 << PSW_I); // Замаскировать прерывания IRQ
    ((union KEY_STATE *)REG_KEY_STATE)->bits.INT_MASK = 1; // Отключить прерывание от клавиатуры

    sp_paint_brick(0, 0, 64, FIELD_Y_OFFSET, 0); // Очистить экран с верха до начала игрового поля

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
                            bug->state = CREATURE_STARTING;
                            bug->count = 6; // Время до запуска врага
                            bug->wait = 0;  // Враг не задержан
                            bug->image_phase = 0;
                            bug->image_phase_inc = 1;
                            bug->x_graph = bug_start_x;
                            bug->y_graph = bug_start_y;
                            bug->dead_bag = 0;
                            bug->type = BUG_NOBBIN;      // Враги рождаются в виде Ноббинов
                            bug->dir = DIR_LEFT;

                            bugs_active++;  // Увеличить счётчик активных врагов
                            bugs_created++; // Увеличить общее количество созданных врагов

                            break;
                        }
                    }
                }
                else
                {
                    if ((bonus_state == BONUS_OFF) && (bugs_created == bugs_total)) // Если Бонус (вишенка) ещё не появлялся и создано максимальное количество врагов
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

            if (bug->type == BUG_NOBBIN) // Если это Ноббин
            {
                for (uint16_t t = 0; t < bugs_max; ++t)
                {
                    if (t == i) continue; // Пропустить самого себя

                    struct bug_info *another_bug = &bugs[t];
                    if (another_bug->state != CREATURE_ALIVE) continue; // Пропустить неживых врагов

                    // Если враг соприкоснулся с другим врагом, увеличить счётчик застревания
                    if (check_collision(bug->x_graph, bug->y_graph, another_bug->x_graph, another_bug->y_graph, 4, 15)) bug->count++;
                }

                //  Если Ноббин застрял или соприкоснулся с другим на определённое (зависящее от уровня сложности) время
                if (bug->count > (20 - difficulty))
                {
                    bug->count = 0;         // Сбросить счётчик застревания
                    bug->type = BUG_HOBBIN; // Переключить тип врага на Хоббина
                }
            }

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

                    if (bug->type == BUG_NOBBIN) // Если враг Ноббин
                    {
                        // Если выпало случайное число с вероятностью зависящей от уровня сложности
                        if ((rand() & 0xF) < difficulty) move_bug(bug); // Переместить врага ещё раз для увеличения скорости
                    }
                }

                case CREATURE_STARTING: // Враг ждёт старта
                {
                    move_bug(bug); // Переместить врага
                    break;
                }

                case CREATURE_DEAD_MONEY_BAG: // Враг погиб от мешка с деньгами и летит вместе с ним
                {
                    uint8_t bag_y_pos = bug->dead_bag->y_graph; // Вертикальная позиция мешка от которого погиб враг
                    if (bag_y_pos + MOVE_Y_STEP > bug->y_graph)
                    {
                        erase_4_15(bug->x_graph, bug->y_graph);
                        bug->y_graph = bag_y_pos; // Если мешок опустился ниже врага, враг перемещается за мешком
                    }

                    switch (bug->type)
                    {
                        case BUG_HOBBIN:
                        {
                            // Нарисовать погибшего Хоббина
                            if (bug->dir == DIR_RIGHT)
                            {
                                sp_4_15_put(bug->x_graph, bug->y_graph, (uint8_t *)image_hobbin_right_dead);
                            }
                            else
                            {
                                sp_4_15_h_mirror_put(bug->x_graph, bug->y_graph, (uint8_t *)image_hobbin_right_dead);
                            }

                            break;
                        }

                        case BUG_NOBBIN:
                        {
                            // Нарисовать погибшего Ноббина
                            sp_4_15_put(bug->x_graph, bug->y_graph, (uint8_t *)image_nobbin_dead);
                            break;
                        }
                    }

                    bug->count = 1;
                    bug->state = CREATURE_RIP; // Враг лежит дохлый
                    add_score(250); // 250 очков за убитого врага

                    break;
                }

                case CREATURE_RIP: // Враг лежит дохлый
                {
                    if (bug->count)
                    {
                        bug->count--; // Уменьшить счётчик дохлого врага
                        break;
                    }

                    // Стирание убитого врага
                    erase_4_15(bug->x_graph, bug->y_graph);
                    bug->state = CREATURE_INACTIVE; // Декативировать убитого врага
                    bugs_active--;   // Уменьшить количество активных врагов

                    // Количество оставшихся врагов (сколько осталось создать плюс количество активных)
                    uint8_t creatures_left =  bugs_total - bugs_created + bugs_active;
                    if (!creatures_left) done_snd = 1; // Если врагов больше не осталось - окончание уровня

                    break;
                }
            }
        }

        // Обработка мешков
        uint8_t man_abs_x_pos = man_x_graph - FIELD_X_OFFSET;
        uint8_t man_abs_y_pos = man_y_graph - FIELD_Y_OFFSET;
        uint8_t man_x_log = man_abs_x_pos / POS_X_STEP;
        uint8_t man_y_log = man_abs_y_pos / POS_Y_STEP;

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
                        if (background[bag_y_log + 1][bag_x_log] != 0xFF) // Если клетка ниже повреждена
                        {
                            switch (bag->dir)
                            {
                                case DIR_STOP: // Если мешок неподвижен
                                {
                                    // Если Диггер двигался вверх и он находится под мешком, то пока не начинать раскачивать мешок
                                    if (!(man_dir == DIR_UP &&  (man_x_log == bag_x_log && (man_y_log == bag_y_log + 1))))
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

                            // Нарисовать спрайт раскачивающегося мешка
                            sp_put(bag_x_graph, bag_y_graph, sizeof(image_bag_left[0]), sizeof(image_bag_left) / sizeof(image_bag_left[0]), bag_image, bag_outline);
                        }
                    }
                    else
                    {
                        // Если счётчик времени до падения мешка закончился
                        bag->state = BAG_FALLING; // Начать падение мешка
                        bag->dir = DIR_DOWN; // Направление движения мешка - вниз
                        bag->count = 0;
                        fall_snd = 1; // Включить звук падения мешка
                    }

                    break;
                }

                case BAG_FALLING: // Мешок падает
                {
                    bags_fall = 1; // Найден падающий мешок

                    // Стереть фон и сбросить биты матрицы фона
                    era_up(bag_x_graph, bag_y_graph + 9);
                    era_background(bag_x_graph, bag_y_graph, DIR_DOWN); // Сбросить биты матрицы фона

                    // Стереть падающий мешок по старым координатам
                    if (bag->count) // Если номер этажа не нулевой
                    {
                        // Если пролетел больше одного этажа, то стираем прямоушольниками
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

                        if (bag_y_log == H_MAX - 1) stop_bag(bag); // Остановить мешок, если он долетел до последнего этажа
                        else if (background[bag_y_log + 1][bag_x_log] == 0xFF) stop_bag(bag); // Остановить мешок, если клетка под ним не повреждена
                    }

                    remove_coin(bag_x_log, bag_y_log); // Удалить монету в клетке куда попал мешок

                    // TODO: Удалить мешки встречающиеся на пути (dest_coin(x, y))

                    // Нарисовать падающий мешок
                    sp_put(bag_x_graph, bag_y_graph, sizeof(image_bag_fall[0]), sizeof(image_bag_fall) / sizeof(image_bag_fall[0]),
                           (uint8_t *)image_bag_fall, (uint8_t *)outline_bag_fall);

                    if (man_state == CREATURE_ALIVE) //  Если Диггер жив
                    {
                        // Проверить, что Диггер попал под падающий под мешок
                        if (check_collision(man_x_graph, man_y_graph, bag_x_graph, bag_y_graph, 4, 12))
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
                        if (check_collision(bug->x_graph, bug->y_graph, bag_x_graph, bag_y_graph, 4, 15))
                        {
                            bug->state = CREATURE_DEAD_MONEY_BAG; // Враг был убит мешком с деньгами
                            bug->dead_bag = bag; // Указатель на мешок которым был убит враг

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
                    // Анимация рабивающегося мешка (три фазы, пропуская один такт счётчика)
                    if (bag->count++ < 6)
                    {
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
                        break_bag_snd = 1; // Издать звук разбившегося мешка
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

        if (fall_snd)
        {
            if (!bags_fall)
            {
                fall_snd = 0;
            }
        }
        else
        {
            if (bags_fall)
            {
                fall_period = 1024;
                fall_snd_phase = 0;
                fall_snd = 1;
            }
        }

        if (!bags_loose) // Если ни один мешок не качается
        {
            loose_snd = 0;  // Остановить звук качающегося мешка
        }

        // Обработка снаряда
        // if (mis_explode) explode_missile(); // Обработка взрывающегося снаряда
        // else move_missile(); // Обработка летящего снаряда

        // Обработка перемещения Диггера
        if (man_state == CREATURE_ALIVE) // Если Диггер жив
        {
            if (man_wait) // Если Диггер в режиме задержки (при толкании мешков)
            {
                // Нарисовать Диггера
                draw_man();
                man_wait--;
            }
            else move_man(); // Переместить и нарисовать Диггера
        }
        else
        {
            // Перемещение и отрисовка убитого Диггера
            if (man_state == CREATURE_DEAD_MONEY_BAG)
            {
                uint8_t bag_y_pos = man_dead_bag->y_graph; // Вертикальная позиция мешка от которого погиб Диггер
                if (bag_y_pos > man_y_graph)
                {
                    erase_4_15(man_x_graph, man_y_graph);
                    man_y_graph = bag_y_pos; // Если мешок опустился ниже Диггера, Диггер перемещается за мешком
                }

                // Нарисовать перевёрнутого Диггера
                sp_4_15_put(man_x_graph, man_y_graph, (uint8_t *)image_digger_turned_over);

                if (man_dead_bag->dir == DIR_STOP)
                {
                    man_state = CREATURE_RIP;
                    man_y_graph -= MOVE_Y_STEP;
                    man_rip_snd = 1;
                }
            }
        }

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
                    sp_paint_brick(0, BONUS_IND_START, SCREEN_BYTE_WIDTH, SCREEN_PIX_HEIGHT - BONUS_IND_START, (bonus_time & 1) ? 0xFF : 0x00);

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

        // Декрементировать таймер между последовательными съедениями драгоценных камней (монеток)
        if (coin_time > 0) coin_time--;
        else coin_snd_note = -1;

        sound_effect();

        if (man_state == CREATURE_AFTER_RIP)
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
                // TODO: Game Over
                init_game(); // Установить игру в начальное состояние
            }
        }

        if (done_snd)
        {
            done_snd = 0;

            // Циклическое увеличение номера уровня
            level_no++;
            level_no &= LEVELS_NUM - 1;

            if (difficulty < 10) difficulty++; // Увеличение сложности после прохождения очередного уровня

            init_level(); // Инициализация нового уровня
        }

        while ((tve_csr->reg & (1 << TVE_CSR_FL)) == 0); // Ожидать срабатывания таймера.
    }
}
