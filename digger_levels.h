#pragma once
#include <stdint.h>

#define W_MAX 15 // Количество ячеек в уровне по-горизонтали
#define H_MAX 10 // Количество ячеек в уровне по-вертикали
#define LEVELS_NUM 8 // Количество уровней

#define CELLS_PER_WORD 5 // Количество ячеек уровня в одном машинном слове

/**
 * @brief Перечисление значений ячейки поля уровня.
 */
enum level_symbols
{
    LEV_SP = 0, // Пустое поле
    LEV_C,      // Монетка (камешек)
    LEV_B,      // Мешок с деньгами
    LEV_H,      // Горизонтальный проход
    LEV_V,      // Вертикальный проход
    LEV_S       // Вертикальный и горизонтальный (гнездо врагов)
};

/**
 * @brief Поле ячеек уровней
 */
extern const uint16_t level[LEVELS_NUM][H_MAX][W_MAX / CELLS_PER_WORD];
