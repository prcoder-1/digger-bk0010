#pragma once
#include <stdint.h>

/**
 * @brief Диггер перемещается вправо
 */
extern const uint8_t image_digger_right[6][15][4];
// extern const uint8_t outline_digger_right[6][15][4];

/**
 * @brief Диггер перемещается влево
 */
// extern const uint8_t image_digger_left[6][15][4];
// extern const uint8_t outline_digger_left[6][15][4];

/**
 * @brief Диггер перемещается вверх
 */
extern const uint8_t image_digger_up[6][15][4];
// extern const uint8_t outline_digger_up[6][15][4];

/**
 * @brief Диггер перемещается вниз
 */
// extern const uint8_t image_digger_downp[6][15][4];
// extern const uint8_t outline_digger_down[6][15][4];

/**
 * @brief Ноббин
 */
extern const uint8_t image_nobbin[3][15][4];
// extern const uint8_t outline_nobbin[3][15][4];

/**
 * @brief Падающий дохлый Ноббин
 */
extern const uint8_t image_nobbin_dead[15][4];
// extern const uint8_t outline_nobbin_dead[15][4];

/**
 * @brief Хоббин перемещающийся вправо
 */
extern const uint8_t image_hobbin_right[3][15][4];
// extern const uint8_t outline_hobbin_right[3][15][4];


/**
 * @brief Падающий дохлый Хоббин смотрящий вправо
 */
extern const uint8_t image_hobbin_right_dead[15][4];
// extern const uint8_t outline_hobbin_right_dead[15][4];

/**
 * @brief Хоббин перемещающийся влево
 */
// extern const uint8_t image_hobbin_left[3][15][4];
// extern const uint8_t outline_hobbin_left[3][15][4];

/**
 * @brief Падающий дохлый Хоббин смотрящий влево
 */
// extern const uint8_t image_hobbin_left_dead[14][4];
// extern const uint8_t outline_hobbin_left_dead[14][4];

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
// extern const uint8_t outline_bag_broke[3][15][4];

/**
 * @brief Надгробный камень
 */
extern const uint8_t image_rip[5][15][4];
// extern const uint8_t outline_rip[5][15][4];

/**
 * @brief Перевёрнутый Диггер
 */
extern const uint8_t image_digger_turned_over[15][4];
// extern const uint8_t outline_digger_turned_over[15][4];

/**
 * @brief Драгоценный камень (монетка)
 */
extern const uint8_t image_coin[10][4];
extern const uint8_t outline_coin[10][4];

/**
 * @brief Спрайт для стирания драгоценного камня (монетки)
 */
// extern const uint8_t image_no_coin[10][4];
extern const uint8_t outline_no_coin[10][4];

/**
 * @brief Летящий снаряд
 */
// extern const uint8_t image_missile[3][7][2];
// extern const uint8_t outline_missile[3][7][2];

/**
 * @brief Взрывающийся снаряд
 */
// extern const uint8_t image_explode[3][7][2];
// extern const uint8_t outline_explode[3][7][2];

/**
 * @brief Вишенка (бонус)
 */
extern const uint8_t image_cherry[15][4];
// extern const uint8_t outline_cherry[15][4];

/**
 * @brief Маска для стирания фона вправо
 */
// extern const uint8_t image_blank_right[18][2];
extern const uint8_t outline_blank_right[18][2];

/**
 * @brief Маска для стирания фона влево
 */
// extern const uint8_t image_blank_left[18][2];
extern const uint8_t outline_blank_left[18][2];

/**
 * @brief Маска для стирания фона вверх
 */
// extern const uint8_t image_blank_up[6][6];
extern const uint8_t outline_blank_up[6][6];

/**
 * @brief Маска для стирания фона вниз
 */
// extern const uint8_t image_blamk_down[6][6];
extern const uint8_t outline_blamk_down[6][6];

/**
 * @brief Маска для стирания в начале полёта мешка
 */
// extern const uint8_t image_bag_falling_start[6][6];
// extern const uint8_t outline_bag_falling_start[6][6];

/**
 * @brief Маска для стирания во время полёта мешка
 */
// extern const uint8_t image_bag_falling[8][6];
// extern const uint8_t outline_bag_falling[8][6];

/**
 * @brief Образцы фона
 */
extern const uint8_t image_background[8][4][5];
// extern const uint8_t outline_background[8][4][5];

/**
 * @brief Жизнь для первого игрока (смотрит вправо)
 */
// extern const uint8_t image_life_right[12][4];
// extern const uint8_t outline_life_right[12][4];

/**
 * @brief Жизнь для второго игрока (смотрит влево)
 */
// extern const uint8_t image_life_left[12][4];
// extern const uint8_t outline_life_left[12][4];

/**
 * @brief Пустой спрайт
 */
// extern const uint8_t image_empty[12][4];
// extern const uint8_t outline_empty[12][4];
