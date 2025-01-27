#pragma once
#include <stdint.h>

#define nullptr 0

/**
 * @brief Распределение адресного пространства БК 0010
 */
enum MEMORY : uint16_t
{
    MEM_SYSTEM  = 0000000, /**< Системные переменные */
    MEM_STACK   = 0000300, /**< Стек */
    MEM_USER    = 0001000, /**< ОЗУ пользователя */
    MEM_VIDEO   = 0040000, /**< ОЗУ экрана */
    MEM_EXTMEM  = 0070000, /**< ОЗУ экрана в режиме расширенной памяти */
    MEM_ROM_MON = 0100000, /**< ПЗУ монитора и драйверы */
    MEM_ROM_1   = 0120000, /**< 1-съёмное ПЗУ "Бейсик" (Фокал) */
    MEM_ROM_2   = 0140000, /**< 2-съёмное ПЗУ "Бейсик" */
    MEM_ROM_3   = 0160000, /**< 3-съёмное ПЗУ "Бейсик" (Тесты МСТД) */
    MEM_REGS    = 0177600, /**< Область системных регистров */
    MEM_END     = 0177777  /**< Конец памяти */
};

/**
 * @brief Вектора прерываний БК 0010
 */
enum VECTORS : uint16_t
{
    VEC_STOP          = 000004, /**< Нажатие клавиши "СТОП" или зависание при передаче данных по каналу */
    VEC_RES_CPU_INSTR = 000010, /**< Зарезервированная инструкция процессора */
    VEC_T_BIT         = 000014, /**< Прерывание по Т-разряду */
    VEC_IOT           = 000020, /**< Прерывание по команде IOT */
    VEC_POWER_FAIL    = 000024, /**< Авария сетевого питания */
    VEC_EMT           = 000030, /**< Прерывание по команде EMT */
    VEC_TRAP          = 000034, /**< Прерывание по команде TRAP */
    VEC_KEYBOARD      = 000060, /**< Прерывание от клавиатуры */
    VEC_IRQ2          = 000100, /**< Сигнал IRQ2 */
    VEC_KEY_LOW_REG   = 000274  /**< Прерывание от клавиатуры (код нижнего регистра) */
};

/**
 * @brief Системные ячейки памяти БК 0010
 */
enum SYSTEM : uint16_t
{
    /** Ячейки соответствующие слову состояния дисплея */
    SYS_COLOR = 040, /**< Цветной режим (32 символа в строке) */
    SYS_SCR_INVERSE, /**< Инверсия экрана (фона) */
    SYS_EXT_MEMORY,  /**< Режим расширенной памяти */
    SYS_RUS,         /**< Режим расширенной памяти */
    SYS_UNDERLINE,   /**< Подчёркивание символов */
    SYS_SYM_INVERSE, /**< Инверсия символов */
    SYS_IND_SU,      /**< Индикация символов управления (ИНД СУ) */
    SYS_BLOCK_RED,   /**< Блокировка редактирования */
    SYS_GRAPH,       /**< Режим текстовой графики "ГРАФ" */
    SYS_ZAP,         /**< Запись в режиме "ГРАФ ("ЗАП") */
    SYS_STIR,        /**< Стирание в режиме "ГРАФ" ("СТИР") */
    SYS_S_LINE_32,   /**< Режим "32 символа в служебной строке" */
    SYS_S_LINE_UND,  /**< Подчёркивание символоа в служебной строке */
    SYS_S_LINE_INV,  /**< Инверсия символа в служебной строке */
    SYS_CURSOR_OFF,  /**< Гашение курсора */
    SYS_UNUSED,      /**< Не используется */

    SYS_LAST_KEY = 0104, /**< Код последнего введённого с клавиатуры символа */
    SYS_LAST_KEY_FLAG,   /**< Признак записи кодд последнего введённого символа */
    SYS_KEY_REPEAT_RATE, /**< Количество повторов «пустого» цикла SOB при нажатии клавиши «ПОВТ» */

    SYS_PAR_INTERF_SHADOW = 0256,

    SYS_EMT_36_PARAMS = 0320, /** Блок параметров работы с магнитофоном (struct EMT_36_PARAMS) */
};

/**
 * @brief Системные регистры БК 0010
 */
enum REGISTERS : uint16_t
{
    REG_KEY_STATE  = 0177660, /**< Регистр состояния клавиатуры */
    REG_KEY_DATA   = 0177662, /**< Регистр данных клавиатуры */
    REG_V_SCROLL   = 0177664, /**< Регистр вертикального смещения изображения */
    REG_TVE_LIMIT  = 0177706, /**< Регистр значения перезагрузки и захвата (запись/чтение) */
    REG_TVE_COUNT  = 0177710, /**< Регистр счётчика таймера (только чтение) */
    REG_TVE_CSR    = 0177712, /**< Регистр управления таймера */
    REG_PAR_INTERF = 0177714, /**< Регистр параллельного интерфейса (порта ввода-вывода) */
    REG_EXT_DEV    = 0177716  /**< Регистр управления системными внешними устройствами */
};

/**
 * @brief Номера битов регистра состояния клавиатуры
 */
enum KEY_STATE_BITS
{
    KEY_STATE_INT_MASK = 6, /**< Маска прерываний от клавиатуры */
    KEY_STATE_STATE         /**< Флаг состояния клавиатуры */
};

/**
 * @brief Регистр состояния клавиатуры
 */
union KEY_STATE
{
    volatile struct
    {
        uint8_t UNUSED_1 : 6; /**< Не используются */
        uint8_t INT_MASK : 1; /**< Маска прерываний от клавиатуры */
        uint8_t STATE    : 1; /**< Флаг состояния клавиатуры */
    } bits;

    volatile uint8_t reg; /**< Значение в виде байта */
};

/**
 * @brief Номера битов регистра вертикального смещения изображения
 */
enum V_SCROLL_BITS
{
    V_SCROLL_EXT_MEMORY = 9
};

/**
 * @brief Регистр вертикального смещения изображения
 */
union V_SCROLL
{
    volatile struct
    {
        uint16_t ADDRESS    : 8; /**< Адрес экранного ОЗУ (исходное состояние 330) */
        uint16_t UNUSED_1   : 1; /**< Не используются */
        uint16_t EXT_MEMORY : 1; /**< Режим расширенной памяти */
        uint16_t UNUSED_2   : 6; /**< Не используются */
    } bits;

    volatile uint16_t reg; /**< Значение в виде 16-битного числа */
};

/**
 * @brief Номера битов регистра управления таймера
 */
enum TVE_CSR_BITS
{
    TVE_CSR_SP = 0, /**< Выбор режим тактирования счетчика таймера по ниспадающему фронту на входе nSP */
    TVE_CSR_CAP,    /**< Режим захвата. При нулевом значении бита CAP таймер генерирует
                     * событие при переходе счетчика через значение нуль и осуществляет
                     * загрузку регистра счетчика из регистра предела. */
    TVE_CSR_MON,    /**< Разрешить мониторинг события таймера */
    TVE_CSR_OS,     /**< Использовать однократный режим, при достижении счетчиком нуля,
                     * бит RUN будет сброшен, таймер перезагружен и остановлен */
    TVE_CSR_RUN,    /**< Запись единицы разрешает работу счетчика */
    TVE_CSR_D16,    /**< Разрешить делитель частоты на 16 */
    TVE_CSR_D4,     /**< Разрешить делитель частоты на 4 */
    TVE_CSR_FL,     /**< Флаг события таймера */
};

# define TMR_FREQ 23438

/**
 * @brief Регистр управления таймера
 *
 * Режимы работы таймера:
 * RUN | OS | CAP | Режим
 *  0  |  x |  x  | Остановлен
 *  1  |  0 |  0  | Непрерывный счет с перезагрузкой счетчика
 *  1  |  1 |  0  | Однократный счет с предзагрузкой счетчика
 *  1  |  x |  1  | Непрерывный счет без перезагрузки счетчика, захват значения счетчика по входу nSP
 *
 * Тактовая частота таймера clk = f/128 = 3 МГц / 128 = 23437,5 Гц, где f это частота на которой работает процессор.
 */
union TVE_CSR
{
    volatile struct
    {
        uint8_t SP     : 1; /**< Выбор режим тактирования счетчика таймера по ниспадающему фронту на входе nSP */
        uint8_t CAP    : 1; /**< Режим захвата. При нулевом значении бита CAP таймер генерирует
                              * событие при переходе счетчика через значение нуль и осуществляет
                              * загрузку регистра счетчика из регистра предела. */
        uint8_t MON    : 1; /**< Разрешить мониторинг события таймера */
        uint8_t OS     : 1; /**< Использовать однократный режим, при достижении счетчиком нуля,
                              * бит RUN будет сброшен, таймер перезагружен и остановлен */
        uint8_t RUN    : 1; /**< Запись единицы разрешает работу счетчика */
        uint8_t D16    : 1; /**< Разрешить делитель частоты на 16 */
        uint8_t D4     : 1; /**< Разрешить делитель частоты на 4 */
        uint8_t FL     : 1; /**< Флаг события таймера */
    } bits;

    volatile uint8_t reg; /**< Значение в виде байта */
};

/**
 * @brief Номера битов регистра параллельного интерфейса (порта ввода-вывода)
 *        (Для использования совместно с манипулятором игровым "Электроника")
 */
enum PAR_INTERF_BITS
{
    PAR_INTERF_UP = 0,
    PAR_INTERF_RIGHT,
    PAR_INTERF_DOWN,
    PAR_INTERF_LEFT,
    PAR_INTERF_A,
    PAR_INTERF_LEFT_BUTTON,
    PAR_INTERF_RIGHT_BUTTON,
    PAR_INTERF_B
};

/**
 * @brief Номера битов регистра управления системными внешними устройствами
 */
enum EXT_DEV_BITS
{
    EXT_DEV_LINE = 4,   /**< Передача/приём информации на/с линии (исходное состояние 1) */
    EXT_DEV_LINE_RDY,   /**< Передача/приём сигнала готовности на/с линию */
    EXT_DEV_MAG_KEY,    /**< Передача информации на магнитофон и сигнала на пьезодинамик / Индикатор нажатия клавиши (0 - нажата, 1 - отжата) */
    EXT_DEV_MOTOR_RDY,  /**< Включение мотора магнитофона (активный 0) / Чтение сигнала готовности с линии */
    EXT_DEV_RESET_VECT, /**< Адрес пуска процессора при включении питания (старший байт, младший = 0; только для чтения) */
};

/**
 * @brief Регистр управления системными внешними устройствами
 */
union EXT_DEV
{
    volatile struct
    {
        uint16_t CPU_MODE   : 4; /**< Режим работы процессора (только чтение) */
        uint16_t LINE       : 1; /**< Передача/приём информации на/с линии (исходное состояние 1) */
        uint16_t LINE_RDY   : 1; /**< Передача/приём сигнала готовности на/с линию */
        uint16_t MAG_KEY    : 1; /**< Передача информации на магнитофон и сигнала на пьезодинамик / Индикатор нажатия клавиши (0 - нажата, 1 - отжата) */
        uint16_t MOTOR_RDY  : 1; /**< Включение мотора магнитофона (активный 0) / Чтение сигнала готовности с линии */
        uint16_t RESET_VECT : 8; /**< Адрес пуска процессора при включении питания (старший байт, младший = 0; только для чтения) */
    } bits;

    volatile uint16_t reg; /**< Значение в виде 16-битного числа */
};

/**
 * @brief Константы для вывода изображения на экран
 */
enum SCREEN : uint16_t
{
    SCREEN_WORD_WIDTH      = 32,  /**< Ширина экрана в словах */
    SCREEN_BYTE_WIDTH      = 64,  /**< Ширина экрана в байтах */
    SCREEN_PIX_HEIGHT      = 256, /**< Высота экрана в пикселях */

    SCREEN_CLR_PX_PER_Byte = 4,   /**< Количество пикселей в байте в цветном режиме */
    SCREEN_CLR_PX_PER_WORD = 8,   /**< Количество пикселей в слове в цветном режиме */
    SCREEN_CLR_PIX_WIDTH   = 256, /**< Ширина экрана в пикселях в цветном режиме */

    SCREEN_BW_PX_PER_BYTE  = 8,   /**< Количество пикселей в байте в чёрно-бклом режиме */
    SCREEN_BW_PX_PER_WORD  = 16,  /**< Количество пикселей в слове в чёрно-бклом режиме */
    SCREEN_BW_PIX_WIDTH    = 512, /**< Ширина экрана в пикселях в чёрно-белом режиме */
};
