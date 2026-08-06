#include "memory.h"
#include "sprites.h"
#include "sound.h"
#include "tools.h"
#include "emt.h"
#include "digger_sprites_title.h"
#include "digger_music_title.h"
#include "digger_full_font.h"
#include "digger_title.h"
#include "digger_credits.h"

#define STR_(x) #x
#define STR(x) STR_(x)

#define COIN_Y_OFFSET 3 // Смещение спрайта монетки в ячейке по оси Y

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
static void print_str(const char *str, uint16_t x_graph, uint16_t y_graph)
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

const char version_str[] = "VERSION " STR(VERSION);
constexpr uint16_t version_str_x_pos = (SCREEN_BYTE_WIDTH - char_width * sizeof(version_str) + char_width) / 2;
constexpr uint16_t version_str_y_pos = str_height + y_space + 2 + table_height + y_space;

// Дата сборки в виде ДД.ММ.ГГГГ передаётся из Makefile (-DBUILD_DATE)
const char build_date_str[] = STR(BUILD_DATE);
constexpr uint16_t build_date_x_pos = (SCREEN_BYTE_WIDTH - char_width * sizeof(build_date_str) + char_width) / 2;
constexpr uint16_t build_date_y_pos = version_str_y_pos + str_height + y_space;

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
static void init_demo()
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
bool hobbin_mirror, digger_mirror;
uint8_t image_phase;        ///< Фаза анимации при выводе спрайта
int8_t image_phase_inc = 1; ///< Направление изменения фазы анимации при выводе спрайта (+1 или -1)

// Таблица действующих лиц демо. Все трое проходят одинаковый таймлайн,
// различаются только спрайтами, координатами, подписью и наличием разворота
// (у Ноббина зеркального спрайта нет).
struct demo_walker
{
    uint16_t *x, *y;
    bool *mirror;                   ///< nullptr — актёр не разворачивается
    const uint8_t (*frames)[15][4]; ///< 3 фазы анимации
    const char *name;
};

static const struct demo_walker walkers[] = {
    { &nobbin_x, &nobbin_y, nullptr,        image_nobbin,       "NOBBIN" },
    { &hobbin_x, &hobbin_y, &hobbin_mirror, image_hobbin_right, "HOBBIN" },
    { &digger_x, &digger_y, &digger_mirror, image_digger_right, "DIGGER" },
};

constexpr uint8_t walker_count = sizeof(walkers) / sizeof(walkers[0]); ///< Число действующих лиц демо

// Таблица статичных объектов демо
struct demo_item
{
    const uint8_t *image;
    const char *name;
    uint8_t height;   ///< Высота спрайта в строках
    uint8_t y_offset; ///< Смещение спрайта в ячейке по оси Y
};

static const struct demo_item items[] = {
    { (const uint8_t *)image_bag,    "GOLD",    sizeof(image_bag) / sizeof(image_bag[0]),    0 },
    { (const uint8_t *)image_coin,   "EMERALD", sizeof(image_coin) / sizeof(image_coin[0]),  COIN_Y_OFFSET },
    { (const uint8_t *)image_cherry, "BONUS",   sizeof(image_cherry) / sizeof(image_cherry[0]), 0 },
};

constexpr uint8_t item_count = sizeof(items) / sizeof(items[0]); ///< Число статичных объектов демо

bool panel_keys = false; ///< Какую панель показывать в этом цикле демо: false — кредиты, true — клавиши

static const char blank_row[CREDITS_FIELD + 1] = "               "; // Пробелы на всю ширину поля панели

/**
 * @brief Вывод текстовой панели (кредиты / описание клавиш) в левое поле
 *        драйвером дисплея ПЗУ (EMT), строки центрированы
 *
 * Панели разной длины и с разной шириной строк, поэтому каждая из CREDITS_ROWS
 * строк поля сначала гасится печатью пробелов — это ровно те же знакоместа,
 * что занимает текст, без привязки к пиксельной геометрии знакоместа драйвера
 * (paint_brick задел бы верхнюю линию рамки).
 *
 * @param p - текст панели: строки <= CREDITS_FIELD знакомест, разделитель '\n'
 */
static void print_panel(const char *p)
{
    for (uint8_t i = 0; i < CREDITS_ROWS; ++i)
    {
        uint8_t row = CREDITS_ROW0 + i;

        // Гашение строки поля
        EMT_24(CREDITS_MARGIN, row);
        EMT_20_l(blank_row, CREDITS_FIELD);

        if (!*p) continue; // Текст кончился — строка остаётся пустой

        const char *e = p;
        uint8_t len = 0;
        while (*e && *e != '\n')
        {
            uint8_t b = (uint8_t)*e++;
            if (b < 0200 || b > 0237) ++len; // печатаемый символ (не управляющий код)
        }

        EMT_24(CREDITS_MARGIN + (CREDITS_FIELD - len) / 2, row);
        // Строка передаётся драйверу одним EMT 020 до разделителя '\n'
        // (сам '\n' тоже уходит драйверу — безвредно, EMT_24 выше
        // репозиционирует каждую строку). Лимит длины 255 — страховка.
        EMT_20_l(p, ((uint16_t)'\n' << 8) | 255);
        p = e;

        if (*p == '\n') ++p; // Пропустить разделитель строк
    }

    // Вывод текста заново включает курсор — гасим его (стираем блок в конце текста).
    // 0232 переключает курсор, поэтому шлём только если он сейчас включён.
    if (!EMT_34().bits.CURSOR_OFF) EMT_16(0232);
}

/**
 * @brief Обработка общего состояния демо
 *
 * Все шагающие актёры проходят один и тот же таймлайн внутри своего окна
 * walker_span тактов: 0 — появление у правого края, 1..185 — движение влево,
 * 186 — разворот (у кого есть зеркало), 202 — подпись. Призы — окно item_span:
 * 0 — показ спрайта, 16 — подпись. Табличный код на ~400 байт короче
 * развёрнутого switch по каждому актёру.
 */
static void process_demo_state()
{
    constexpr uint16_t image_width = sizeof(image_nobbin[0][0]);
    constexpr uint16_t image_height = sizeof(image_nobbin[0]) / sizeof(image_nobbin[0][0]); // Высота спрайта Ноббина
    constexpr uint16_t y_space = 8;
    constexpr uint16_t row_step = image_height + y_space; // Шаг строк таблицы демо
    constexpr uint16_t move_start_pos = SCREEN_BYTE_WIDTH - 6;

    constexpr uint16_t move_durance = 184;
    constexpr uint16_t end_to_print = 16;

    // Таймлайн шагающего актёра (смещения внутри его окна)
    constexpr uint16_t walker_move_end = move_durance + 1;          // Конец движения
    constexpr uint16_t walker_mirror = walker_move_end + 1;         // Разворот
    constexpr uint16_t walker_print = walker_mirror + end_to_print; // Подпись
    constexpr uint16_t walker_span = 330;                           // Окно актёра
    constexpr uint16_t walkers_start = 128;                         // Старт первого актёра

    // Таймлайн призов
    constexpr uint16_t item_span = move_durance + end_to_print; // Окно приза
    constexpr uint16_t items_start = walkers_start + (walker_count - 1) * walker_span + walker_print + move_durance;

    // Время до повтора демо
    constexpr uint16_t demo_restart_time = items_start + (item_count - 1) * item_span + end_to_print + 250;

    if (demo_time == 0)
    {
        // Очистка области Demo
        nobbin_x = hobbin_x = digger_x = 0;
        constexpr uint16_t demo_height = table_height - (str_height + y_space * 2) * 2;
        paint_brick(SCREEN_BYTE_WIDTH / 2 + 2, one_player_y, SCREEN_BYTE_WIDTH / 2 - 3, demo_height, 0);

        // Левая панель чередуется по циклам демо: кредиты - клавиши - кредиты
        print_panel(panel_keys ? keys_help : credits);
        panel_keys = !panel_keys;
    }

    // Шагающие актёры: t — смещение внутри окна очередного актёра.
    constexpr uint16_t step_click_pw = 10; // Ширина импульса

    uint16_t t = demo_time - walkers_start;
    for (uint8_t i = 0; i < walker_count; i++, t -= walker_span)
    {
        if (t >= walker_span) continue;
        const struct demo_walker *w = &walkers[i];

        if (t == 0)
        {
            *w->x = move_start_pos;
            *w->y = one_player_y + i * row_step;
            if (w->mirror) *w->mirror = true;
        }
        else if (t <= walker_move_end)
        {
            if (!(demo_time & 7))
            {
                (*w->x)--;
                sound_pwm((demo_time & 8) ? 90 : 60, 2, step_click_pw);
            }
        }
        else if (t == walker_mirror)
        {
            if (w->mirror) *w->mirror = false;
        }
        else if (t == walker_print)
        {
            print_str(w->name, *w->x + image_width * 2 - 1, *w->y);
        }
        break;
    }

    // Призы: появляются под Диггером со сдвигом в строку на каждый
    t = demo_time - items_start;
    for (uint8_t i = 0; i < item_count; i++, t -= item_span)
    {
        if (t >= item_span) continue;
        uint16_t item_y = digger_y + (i + 1) * row_step;

        if (t == 0)
        {
            const struct demo_item *it = &items[i];
            sp_put(digger_x, item_y + it->y_offset, image_width, it->height, it->image, nullptr);
            sound_pwm(60, 7, step_click_pw);
            sound_pwm(90, 5, step_click_pw);
        }
        else if (t == end_to_print)
        {
            print_str(items[i].name, digger_x + image_width * 2 - 1, item_y);
        }
        break;
    }

    // Отрисовка шагающих актёров (пока x ненулевой — актёр на экране)
    for (uint8_t i = 0; i < walker_count; i++)
    {
        const struct demo_walker *w = &walkers[i];
        uint16_t x = *w->x;
        if (!x) continue;

        paint_brick(x + image_width, *w->y, 1, image_height, 0); // Подчистить след справа
        if (w->mirror && *w->mirror) sp_4_15_h_mirror_put(x, *w->y, (const uint8_t *)w->frames[image_phase]);
        else sp_4_15_put(x, *w->y, (const uint8_t *)w->frames[image_phase]);
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

/**
 * @brief Распаковка потока ZX0 в произвольную область памяти.
 *
 * Компактная PDP-11 реализация в `dzx0.s` (распаковщик reddie, 92 байта,
 * + обвязка C ABI) — заметно короче прежней C-версии.
 *
 * @param src - указатель на сжатый поток ZX0
 * @param dst - указатель на буфер-приёмник; он же служит «историей» для ссылок по смещению
 */
void zx0_decompress(const uint8_t *src, uint8_t *dst);

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
        sound_pwm(period, base, pw ? pw : 1); // pw>=1: pw==0 -> sob крутит 65536 итераций
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
    // Подчистить строки "VERSION"/даты сборки: "LOADING" и имя файла
    // печатаются на тех же строках, но другой ширины — без очистки по краям
    // оставались бы хвосты старого текста
    paint_brick(0, loading_str_y_pos, SCREEN_BYTE_WIDTH, str_height * 2 + y_space, 0);

    // Показать строку "LOADING"
    print_str(loading_str, loading_str_x_pos, loading_str_y_pos);

    // Показать строку с именем загружаемого файла
    print_str(game_filename_str, game_filename_x_pos, game_filename_y_pos);

    // Подготовить блок параметров драйвера магнитофона в системной области
    struct EMT_36_PARAMS *p = (struct EMT_36_PARAMS *)SYS_EMT_36_PARAMS;
    p->COMMAND  = EMT_36_FILE_READ;   // Считывание файла через EMT36
    p->DATA_PTR = (uint8_t *)nullptr; // Использовать адрес загрузки из заголовка файла
    p->SIZE     = 0;                  // Размер из заголовка, без ограничения (0)
    // Формирование 16 байт имени файла: копируем символы до NUL, хвост добиваем
    // ПРОБЕЛАМИ (не NUL). Драйвер EMT_36 сравнивает все 16 байт имени с паддингом
    // пробелами; хвостовой NUL дал бы EMT_36_INCORRECT_NAME → игра не загрузилась бы.
    for (uint8_t i = 0; i < 16; i++)
    {
        p->NAME[i] = (i < sizeof(game_filename_str) - 1 && game_filename_str[i])
                     ? game_filename_str[i] // Символ имени файла
                     : ' ';                 // Паддинг пробелом до 16 байт
    }

    // ВНИМАНИЕ: и сам вызов `emt 036`, и последующий переход `jmp @#01000` НЕЛЬЗЯ
    // выполнять из кода заставки (001000..) — загружаемый DIGGER грузится с адреса
    // 001000 и во время загрузки затирает эти самые инструкции, после чего возврат
    // из EMT попадает уже на данные игры.
    // Решение: копируем крошечный стаб «mov #параметры,r1; emt 036; jmp @#01000» в
    // системное ОЗУ НИЖЕ 001000 (эта область загрузкой не затрагивается) и передаём
    // управление туда — стаб переживает загрузку и корректно стартует игру.
    enum { RUN_STUB_ADDR = 0400 }; // свободная системная ячейка: выше блока EMT (0320), ниже стека
    // Стаб на 0400 не должен налезать на блок параметров EMT_36 (0320..)
    static_assert(SYS_EMT_36_PARAMS + sizeof(struct EMT_36_PARAMS) <= RUN_STUB_ADDR,
                  "run_stub overlaps EMT_36 parameter block");
    static const uint16_t run_stub[] = {
        0012701, SYS_EMT_36_PARAMS, // mov #0320, r1  — адрес блока параметров EMT 36
        0104036,                    // emt 036        — загрузка DIGGER поверх 001000..
        0000137, MEM_USER,          // jmp @#01000    — точка входа загруженной игры
    };
    uint16_t *stub = (uint16_t *)RUN_STUB_ADDR;
    for (uint8_t i = 0; i < sizeof(run_stub) / sizeof(run_stub[0]); i++) stub[i] = run_stub[i];

    // Передать управление стабу в системном ОЗУ (не возвращается)
    asm volatile ("jmp (%0)\n" :: "r"((uint16_t)RUN_STUB_ADDR) : "memory");
    __builtin_unreachable();
}

/**
 * @brief Основная программа
 */
void main()
{
    EMT_16(0233); // Цветной режим (32 символа в строке = широкий шрифт)
    EMT_16(0222); // Цвет вывода = зелёный
    // Погасить курсор. Код 0232 ПЕРЕКЛЮЧАЕТ курсор (один код на вкл/выкл — так же
    // делают CURON/CUROFF в ПЗУ), поэтому шлём его, только если курсор сейчас
    // включён (бит CURSOR_OFF в ССД сброшен). Обязательно ПОСЛЕ 0233: смена режима
    // заново включает и рисует курсор в левом верхнем углу — этот вызов его стирает.
    // Иначе курсор оставался бы виден (в углу рамки и в конце текста).
    if (!EMT_34().bits.CURSOR_OFF) EMT_16(0232);

    paint_brick(0, 0, SCREEN_BYTE_WIDTH, SCREEN_PIX_HEIGHT, 0); // Очистка экрана

    set_PSW(1 << PSW_I); // Замаскировать прерывания IRQ
    ((union KEY_STATE *)REG_KEY_STATE)->bits.INT_MASK = 1; // Отключить прерывание от клавиатуры

    // Отображение строки "UNPACKING..."
    // print_str(unpacking_str, unpacking_str_x_pos, unpacking_str_y_pos);

    // Распаковать заставку в экранное ОЗУ
    zx0_decompress(cover_zx0, (uint8_t *)MEM_VIDEO);

    // Воспроизвести музыку Popcorn
    play_popcorn();

    init_demo();    // Инициализация демо (панель печатается первым же вызовом process_demo_state при demo_time == 0)
    print_str(version_str, version_str_x_pos, version_str_y_pos);
    print_str(build_date_str, build_date_x_pos, build_date_y_pos);
    for (;;)
    {
        process_demo_state(); // Обработка состояний демо
        if (any_key_or_button_pressed()) load_and_run_digger(); // Запустить игру при нажатии клавиши или кнопки джойстика
    }
}
