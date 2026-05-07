#pragma once
#include <stdint.h>

/**
 * @brief Диггер перемещается вправо
 */
extern const uint8_t image_digger_right[6][15][4];
// extern const uint8_t outline_digger_right[6][15][4];

/**
 * @brief Диггер перемещается вверх
 */
extern const uint8_t image_digger_up[6][15][4];
// extern const uint8_t outline_digger_up[6][15][4];

/**
 * @brief Ноббин
 */
extern const uint8_t image_nobbin[3][15][4];
// extern const uint8_t outline_nobbin[3][15][4];

/**
 * @brief Хоббин перемещающийся вправо
 */
extern const uint8_t image_hobbin_right[3][15][4];
// extern const uint8_t outline_hobbin_right[3][15][4];

/**
 * @brief Мешок с деньгами наклонившийся влево
 */
extern const uint8_t image_bag_left[15][4];
extern const uint8_t outline_bag_left[15][4];

/**
 * @brief Мешок с деньгами (стоящий на месте)
 */
extern const uint8_t image_bag[15][4];
extern const uint8_t outline_bag[15][4];

/**
 * @brief Мешок с деньгами наклонившийся вправо
 */
extern const uint8_t image_bag_right[15][4];
extern const uint8_t outline_bag_right[15][4];

/**
 * @brief Падающий мешок с деньгами
 */
extern const uint8_t image_bag_fall[15][4];
extern const uint8_t outline_bag_fall[15][4];

/**
 * @brief Разбившися мешок с деньгами
 */
extern const uint8_t image_bag_broke[3][15][4];

/**
 * @brief Надгробный камень
 */
extern const uint8_t image_rip[14][4];

/**
 * @brief Перевёрнутый Диггер
 */
extern const uint8_t image_digger_turned_over[15][4];
extern const uint8_t outline_digger_turned_over[15][4];

/**
 * @brief Драгоценный камень (монетка)
 */
extern const uint8_t image_coin[10][4];
extern const uint8_t outline_coin[10][4];

/**
 * @brief Спрайт для стирания драгоценного камня (монетки)
 */
extern const uint8_t outline_no_coin[10][4];

/**
 * @brief Летящий снаряд
 */
extern const uint8_t image_missile[3][7][2];

/**
 * @brief Взрывающийся снаряд
 */
extern const uint8_t image_explode[3][7][2];

/**
 * @brief Вишенка (бонус)
 */
extern const uint8_t image_cherry[15][4];

/**
 * @brief Маска для стирания фона вправо
 */
extern const uint8_t outline_blank_right[18][2];

/**
 * @brief Маска для стирания фона влево
 */
extern const uint8_t outline_blank_left[18][2];

/**
 * @brief Маска для стирания фона вверх
 */
extern const uint8_t outline_blank_up[6][6];

/**
 * @brief Маска для стирания фона вниз
 */
extern const uint8_t outline_blank_down[6][6];

/**
 * @brief Образцы фона
 */
extern const uint8_t image_background[8][4][5];
