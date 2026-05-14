#pragma once
#include <stdint.h>

/**
 * @brief Выводит спрайт 4x15 байт по заданным координатам в видеопамять
 */
void sp_4_15_put(uint16_t x, uint16_t y, const uint8_t *image);

/**
 * @brief Выводит спрайт 4x15 байт отражённый по-горизонтали по заданным координатам в видеопамять
 */
void sp_4_15_h_mirror_put(uint16_t x, uint16_t y, const uint8_t *image);

/**
 * @brief Выводит спрайт 4x15 байт отражённый по-горизонтали и вертикали по заданным координатам в видеопамять
 */
void sp_4_15_hv_mirror_put(uint16_t x, uint16_t y, const uint8_t *image);

/**
 * @brief Выводит спрайт 4x15 байт с маской по заданным координатам в видеопамять.
 *
 * Специализация для самого частого «масочного» случая. Для чётного адреса
 * приёмника пишет пословно (2 mov на строку), для нечётного — побайтно но без
 * внутреннего sob (4 байтных стора на строку).
 */
void sp_4_15_mask_put(uint16_t x, uint16_t y, const uint8_t *image, const uint8_t *outline);

/**
 * @brief Выводит спрайт произвольного размера по заданным координатам в видеопамять (используя маску)
 */
void sp_put(uint16_t x, uint16_t y, uint16_t x_width, uint16_t y_width, const uint8_t *image, const uint8_t *outline);

/**
 * @brief Стирает блок 16x15 пикселей (4x15 байт)
 *
 * В чётном случае (x_graph чётный) делает 2 пословных clr на строку;
 * в нечётном — 4 побайтных без внутреннего цикла.
 *
 * @param x_graph - координата X блока
 * @param y_graph - координата Y блока
 */
void erase_4_15(uint16_t x_graph, uint16_t y_graph);

/**
 * @brief Стирает прямоугольник по заданным координатам в видеопамяти
 */
void sp_clear_brick(uint16_t x, uint16_t y, uint8_t x_width, uint8_t y_width);
