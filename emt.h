#pragma once
#include <stdint.h>

/**
 * @brief Инициализация драйвера клавиатуры (0.2 мс)
 *
 * По данной команде производится установка векторов прерываний клавиатуры,
 * в регистре состояния сбрасывается маска прерываний от клавиатуры,
 * устанавливается режим передачи кодов по запросам рабочей программы,
 * устанавливается режим передачи кода 012 при нажатии клавиши "ввод".
  */
inline void EMT_4()
{
    asm volatile (
        "emt 04\n"
        : : : "r0", "cc"
    );
}

/**
 * @brief Чтение кода символа с клавиатуры
 *
 * Производится чтение кода с клавиатуры.
 *
 * @return код символа введённого с клавиатуры
*/
inline char EMT_6()
{
    char rv;
    asm volatile (
        "emt 06\n"
        "mov r0, %0"
        : "=r" (rv)
    );

    return rv;
}

/**
 * @brief Чтение строки с клавиатуры
 *
 * Производится воод строки по переданному адресу.
 *
 * @param ptr - адрес по которому будет записана строка
 * @param len_delim - ограничивающее условие (младший байт - максимальная длина строки,\
 * старший - ограничиваюший символ, который будет записан в строку последним)
 */
inline void EMT_10(char *ptr, uint16_t len_delim)
{
    asm volatile (
        "mov %0, r1\n\t"
        "mov %1, r2\n\t"
        "emt 010\n"
        : : "r" (ptr), "r" (len_delim) : "cc"
    );
}

/**
 * @brief Установка ключей клавиатуры
 *
 * Осуществляется программирование ключа АР2 с номером указанным в параметре key_num строкой
 * расположенной по адресу ptr. Для сброса ключа необходимо передать в качестве параметра
 * нулевое значение адреса (NULL). В этом случае реакция на нажатие АР2-ключа будет отсутствовать.
 *
 * @param key_num - номер ключа для программирования
 * @param ptr - указатель на строку, которой будет запрограммирован ключ
 */
inline void EMT_12(uint8_t key_num, char *ptr)
{
    asm volatile (
        "mov %0, r0\n\t"
        "mov %1, r1\n\t"
        "emt 012\n"
        : : "r" (key_num), "r" (ptr) : "r0", "cc"
    );
}

/**
 * @brief Инициализация драйверов системного ПЗУ (240 мс)

 * Команда обеспечивает инициализацию всех драйверов системного ПЗУ, осуществляет сброс
 * рабочих ячеек в исходное состояние, установку всех векторов прерываний, очистку экрана,
 * установку исходных режимов отображения информации, очистку порта ввода-вывода,
 * установку системного порта в исходное состояние, установку скорости обмена
 * по линии 9600 бод. Стек в исходное состояние не устанавливается.
 */
inline void EMT_14()
{
    asm volatile (
        "emt 014\n"
        : : : "r0", "r1", "r2", "r3", "r4", "cc"
    );
}

/**
 * @brief Формирование символов и переключение режимов (0.4 ... 1.7 мс)
 *
 * Команда обеспечивает передачу кодов драйверу дисплея, который формирует символы
 * и обрабатывает управляюшие коды.
 *
 * @param c - код символа или управляющий код
 */
inline void EMT_16(char c)
{
    asm volatile (
        "mov %0, r0\n\t"
        "emt 016\n"
        : : "r" (c) : "cc"
    );
}

/**
 * @brief Формирование строки символов (до нулевого символа) ( (1.6 * N) + 0.5 мс )
 *
 * По данной команде осуществляется передача последовательности кодов драйверу дистплея
 * из области ОЗУ, адрес которой передан в параметрах. Передача кодов прекращается при
 * обнаружении символа с кодом ноль. Символ с кодом ноль, так же, передаётся драйверц.
 *
 * @param ptr - адрес последовательности (строки)
 */
inline void EMT_20(const char *ptr)
{
    asm volatile (
        "mov %0, r1\n\t"
        "clr r2\n\t"
        "emt 020\n"
        : : "r" (ptr) : "r1", "r2", "cc"
    );
}

/**
 * @brief Формирование строки символов (до указанной длины или ограничителя)) ( (1.6 * N) + 0.5 мс )
 *
 * По данной команде осуществляется передача последовательности кодов драйверу дистплея
 * из области ОЗУ, адрес которой передан в параметрах. Передача кодов прекращается при
 * при выполнении одного из органичивающих условий (достижение длины строки или ограничение
 * по символу-ограничителю). Если выполняется ограничени по символу-ограничителю, то
 * последним в строке передаётся код даннного символа.
 *
 * @param ptr - адрес последовательности (строки)
 * @param len_delim - максимальная длина строки (младший байт) и символ-ограничитель (старший байт)
 */
inline void EMT_20_l(const char *ptr, uint16_t len_delim)
{
    asm volatile (
        "mov %0, r1\n\t"
        "mov %1, r2\n\t"
        "emt 020\n"
        : : "r" (ptr), "r" (len_delim) : "r1", "r2", "cc"
    );
}

/**
 * @brief Запись символа в служебную строку
 *
 * По данной команде осуществляется запись символа в указанную позицию служебной строки.
 * Формирование символа производится в соответствии с режимами, действующими в данный
 * момент в основном поле экрана и могут быть отличными от режимов формирования
 * индикаторов.
 *
 * При использовании данной команды следует помнить, что поле индикаторов размещается
 * в правой части служебной стрпоки и максимально может занимать 24 (dec) позиции.
 *
 * @param c - код символа
 * @param pos - позиция символа
 */
inline void EMT_22(char c, uint8_t pos)
{
    asm volatile (
        "mov %0, r0\n\t"
        "mov %1, r1\n\t"
        "emt 022\n"
        : : "r" (c), "r" (pos) : "cc"
    );
}

/**
 * @brief Установка курсора по координатам
 *
 * По данной команду производится цстановка символьного или графического курсора в позицию,
 * заданную координатами X и Y переданными в параметрах. Значениям координат (0,0) соответствует
 * верхняя левая позиция в информационном поле экрана. Максимальные значения координат
 * зависят от размера поля, которое находится в данный момент в распоряжении пользователя.
 * (Под графическим режимом следует понимать режим текстовой графики)
 *
 * @param x - координата X
 * @param y - координата Y
 */
inline void EMT_24(uint8_t x, uint8_t y)
{
    asm volatile (
        "mov %0, r1\n\t"
        "mov %1, r2\n\t"
        "emt 024\n"
        : : "r" (x), "r" (y) : "cc"
    );
}

/**
 * @brief Съём (получение) координат курсора
 *
 * По данной команде осуществляется получение координат символьного или графического
 * (в зависимости от режима) курсора.
 *
 * Координаты возвращаются по адресам переменных X и Y переданных в параметрах.
 *
 * @param - адрес переменной для записи координаты X
 * @param - адрес переменной для записи координаты Y
 */
inline void EMT_26(uint8_t *x, uint8_t *y)
{
    asm volatile (
        "emt 026\n"
        "mov r1, @%0\n\t"
        "mov r2, @%1\n\t"
        : "=r" (x), "=r" (y):  : "cc"
    );
}

/**
 * @brief Формирование точки по координатам
 *
 * По данной команде производится запись или стирание графической точки по координатам,
 * указанным в качестве параметров. Значениям координат (0,0) соответствует верхняя
 * левая точка в информационном поле экрана. Максимальное значение координаты Y = 239 (dec),
 * максимальное значение координаты X зависит от режима, в котором находится в текеущий
 * момент драйвер дисплея. В режиме 64 (dec) символа в строке оно равно 511 (dec),
 * в режиме 32 (dec) символа в строке оно равно 255 (dec). С помощью соответствующих
 * команд можно установить цвет формируемой точки и фона. Для выполнения команды
 * не имеет значения - в символьном режиме или в режиме текстовой графики находится
 * драйвер дисплея.
 *
 * @param w_e - 1 для записи или 0 для стирания
 * @param x - координата X
 * @param y - координата Y
 */
inline void EMT_30(uint8_t w_e, uint8_t x, uint8_t y)
{
    asm volatile (
        "mov %0, r0\n\t"
        "mov %1, r1\n\t"
        "mov %2, r2\n\t"
        "emt 030\n"
        : : "r" (w_e), "r" (x), "r" (y) : "cc"
    );
}

/**
 * @brief Формирование вектора (линии) по координатам
 *
 * По данной команде производится запись или стриание вектора, координаты конца которого
 * указаны в качестве параметров. Координатами начала являются координаты последней сформированной
 * точки (с использованием предыдущей команды) либо координаты конца предыдущего вектора.
 *
 * При формировании вектора действиетльны все условия и ограничения, которые приведены
 * в описании предыдущей команды.
 *
 * Если координаты вектора превышают допустимые значения, то производится формирование
 * только той части вектора, которая имеет допустимые координаты. Таким образом, при формировании
 * излбражения с координатами, превышающими размеры экрана, можно наблюдать только часть
 * изображения, помещающуюся в информационном поле экрана.
 *
 * @param w_e - 1 для записи или 0 для стирания
 * @param x - координата X
 * @param y - координата Y
 */
inline void EMT_32(uint8_t w_e, uint8_t x, uint8_t y)
{
    asm volatile (
        "mov %0, r0\n\t"
        "mov %1, r1\n\t"
        "mov %2, r2\n\t"
        "emt 032\n"
        : : "r" (w_e), "r" (x), "r" (y) : "cc"
    );
}

#pragma pack(push, 1)
/**
 * @brief Слово состояния дисплея (ССД)
 */
union SSD
{
    struct
    {
        uint16_t COLOR       : 1; /**< Цветной режим (32 символа в строке) */
        uint16_t SCR_INVERSE : 1; /**< Инверсия экрана (фона) */
        uint16_t EXT_MEMORY  : 1; /**< Режим расширенной памяти */
        uint16_t RUS         : 1; /**< Регистр "РУС" */
        uint16_t UNDERLINE   : 1; /**< Подчёркивание символов */
        uint16_t SYM_INVERSE : 1; /**< Инверсия символов */
        uint16_t IND_SU      : 1; /**< Индикация символов управления (ИНД СУ) */
        uint16_t BLOCK_RED   : 1; /**< Блокировка редактирования */
        uint16_t GRAPH       : 1; /**< Режим текстовой графики "ГРАФ" */
        uint16_t ZAP         : 1; /**< Запись в режиме "ГРАФ ("ЗАП") */
        uint16_t STIR        : 1; /**< Стирание в режиме "ГРАФ" ("СТИР") */
        uint16_t S_LINE_32   : 1; /**< Режим "32 символа в служебной строке" */
        uint16_t S_LINE_UND  : 1; /**< Подчёркивание символоа в служебной строке */
        uint16_t S_LINE_INV  : 1; /**< Инверсия символа в служебной строке */
        uint16_t CURSOR_OFF  : 1; /**< Гашение курсора */
        uint16_t UNUSED      : 1; /**< Не используется */
    } bits;

    uint16_t reg; /**< Значение в виде 16-битного числа */
};
#pragma pack(pop)

/**
 * @brief Чтение слова состояния дисплея
 *
 * По данной команде производится чтение слова состояния дисплея, отражающего состояние дисплея
 * на текущий момент времени.
 * Под состоянием дисплея понимается совокупность режимов, в которых находится дисплей.
 * Каждый разряд слова состояния отражает состояние соответствующего ему режима.
 * При этом "1" свидетельствует о включенном состоянии данного режима, "0" - о выключенном.
 *
 * @return SSD - слово состояния дисплея
 */
inline union SSD EMT_34()
{
    union SSD rv;
    asm volatile (
        "emt 34\n"
        "mov r0, %0"
        : "=r" (rv.reg)
    );

    return rv;
}

/**
 * @brief Передача управления драйверу магнитофона
 *
 * По данной команде производится запись информации на магнитную ленту или
 * чтение её с магнитной ленты в соответствии с управляющей информацией,
 * указанной в блоке параметров.
 *
 * Блок параметров может быть размещён в произвольной области ОЗУ с чётного адреса,
 * однако, система предоставляет возмождность использовать для размещения блока
 * параметров область ОЗУ с адресами 0320 - 0371 (oct), если есть уверенность,
 * что во время работы драйвера магнитофона не произойдёт взаимного перекрытия
 * блока параметров и системного стека (глубина стека не превышает 16).
 *
 * Для выполнения требуемой операции необходимо предварительно занести нужную
 * информацию в блок параметров и передать адрес блока параметров данной функции.
 *
 * После выполнения операции в байте ответа блока параметров содержится информация
 * о результате выполнения операции.
 *
 * @param ptr - указаьель на блок параметров драйвера магнитофона
 */
inline void EMT_36(const char *ptr)
{
    asm volatile (
        "mov %0, r1\n\t"
        "emt 036\n"
        : : "r" (ptr) : "cc"
    );
}

/**
 * @brief Команды драйвера магнитофона
 */
enum EMT_36_command : uint8_t
{
    EMT_36_RECORDER_STOP = 0, /**< Стоп */
    EMT_36_RECORDER_START,    /**< Пуск двигателя */
    EMT_36_FILE_WRITE,        /**< Запись массива */
    EMT_36_FILE_READ,         /**< Чтение массива */
    EMT_36_FICT_READ          /**< Фиктивное чтение массива */
};

/**
 * @brief Ответы драйвера магнитофона
 */
enum EMT_36_response : uint8_t
{
    EMT_36_OK = 0,            /**< Операция завершена без ошибок */
    EMT_36_INCORRECT_NAME,    /**< Имя текущего файла не совпадает с заданным */
    EMT_36_CRC_ERROR,         /**< Ошибка контрольной суммы */
    EMT_36_STOP               /**< Останов по команде оператора */
};

#pragma pack(push, 1)
/**
 * @brief Блок параметров драйвера магнитофона
 */
struct EMT_36_PARAMS
{
    enum EMT_36_command COMMAND;   /**< Команда драйвера магнитофона */
    enum EMT_36_response RESPONSE; /**< Ответ драйвера магнитофона */
    uint8_t *DATA_PTR;             /**< Адрес массива (файла) на запись/чтение */
    uint16_t SIZE;                 /**< Длина массива (файла) на запись (при чтении нужно обнулить) */
    char NAME[16];                 /**< Имя массива (файла) */
    uint8_t *CUR_DATA_PTR;         /**< Адрес текущего массива (файла) */
    uint16_t CUR_SIZE;             /**< Длина текущего массива (файла) */
    char CUR_NAME[16];             /**< Имя текущего массива (файла) */
};
#pragma pack(pop)

/**
 * @brief Номера скоростей ТЛГ канала
 */
enum EMT_40_speeds : uint8_t
{
    EMT_40_SPEED_9600 = 0, /**< Скорость 9600 бод */
    EMT_40_SPEED_4800,     /**< Скорость 4800 бод */
    EMT_40_SPEED_2400,     /**< Скорость 2400 бод */
    EMT_40_SPEED_1200,     /**< Скорость 1200 бод */
    EMT_40_SPEED_600,      /**< Скорость 600 бод */
    EMT_40_SPEED_300,      /**< Скорость 300 бод */
    EMT_40_SPEED_150,      /**< Скорость 150 бод */
    EMT_40_SPEED_75,       /**< Скорость 75 бод */
    EMT_40_SPEED_50,       /**< Скорость 50 бод */
};

/**
 * @brief Инициализация драйвера ТЛГ (телеграфного) канала
 *
 * Данная команда позволяет установить требуемую скорость обмена по линии ТЛГ.
 * По включению питания автоматически устанавливается скорость обмена 9600 бод.
 *
 * @param speed - Номер скорости обмена информацией
 */
inline void EMT_40(enum EMT_40_speeds speed)
{
    asm volatile (
        "mov %0, r0\n\t"
        "emt 040\n"
        : : "r" (speed) : "cc"
    );
}

/**
 * @brief Передача байта на линии
 *
 * Данная команда обеспечивает передачу байта, переданного в параметре, на линию
 * со скоростью, установленной в данный момент в драйвере последовательного канала.
 * Передача байта начинается с младшего бита, которому предшествует стартовый бит.
 *
 * Перед передачей байта проверяется готовность приёмной стороны к приёму байта.
 * Если сигнал готовности отсутствует, то драйвер переходит в цикл опроса сигнала
 * готовности с линии, из которого выходит только при появлении сигнала готовности,
 * либо по прерыванию по клавише "СТОП".
 *
 * На время передачи байта прерывания от внешних устройств маскируются.
 *
 * @param byte - байт для передачи по линии
 */
inline void EMT_42(uint8_t byte)
{
    asm volatile (
        "mov %0, r0\n\t"
        "emt 040\n"
        : : "r" (byte) : "cc"
    );
}

/**
 * @brief Приём байта с линии
 *
 * Данная команда обеспечивает прийм байта с линии. Скорость передачи на линии должна
 * совпадать со скоростью, установленной в данный момент а драйвере последовательного
 * канала.
 *
 * По данной команда драйвер выдаёт на линию сигнал готовности к приёму байта
 * и переходит в режим ожидания стартового бита. После приёма байта сигнал готовности
 * сбрасывается.
 *
 * На время ожидания байта с линии драйвер разрешает прерывания от внешних устройств.
 * Таким образом, появляется возможность прервать работу драйвера, например с клавиатуры,
 * чтобы получить новый символ и передать его на линию, после чего вернуться в драйвер
 * и ожидать байт.
 *
 * @return uint8_t - байт принятый с линии
 */
inline uint8_t EMT_44()
{
    uint8_t rv;
    asm volatile (
        "emt 044\n"
        "mov r0, %0"
        : "=r" (rv)
    );

    return rv;
}

/**
 * @brief Передача массива по линии
 *
 * Данная команда обеспечивает передачу массива заданной длины, расположенного в ОЗУ
 * по заданному адресу, на линию.
 *
 * @param ptr - указатель на массив, который будет передан
 * @param len - длина массива
 *
 */
inline void EMT_46(const uint8_t *ptr, uint16_t len)
{
    asm volatile (
        "mov %0, r1\n\t"
        "mov %1, r2\n\t"
        "emt 046\n"
        : : "r" (ptr), "r" (len) : "r1", "r2", "cc"
    );
}

/**
 * @brief Приём массива с линии
 *
 * Данная команда обеспечивает приём массива с линии и запись его в ОЗУ по адресу
 * переданному в параметрах.
 *
 * @param ptr - указатель на память, куда будет приням массив с линии
 * @param len - длина массива
 */
inline void EMT_50(uint8_t *ptr, uint16_t len)
{
    asm volatile (
        "mov %0, r1\n\t"
        "mov %1, r2\n\t"
        "emt 010\n"
        : : "r" (ptr), "r" (len) : "cc"
    );
}
