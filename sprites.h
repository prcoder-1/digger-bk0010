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
 * @brief Выводит спрайт произвольного размера по заданным координатам в видеопамять (используя маску)
 */
void sp_put(uint16_t x, uint16_t y, uint16_t x_width, uint16_t y_width, const uint8_t *image, const uint8_t *outline);

/**
 * @brief Закрашивает прямоугольник заданным образцом по заданным координатам в видеопамяти
 */
void sp_paint_brick(uint16_t x, uint16_t y, uint8_t x_width, uint8_t y_width, uint8_t color);
