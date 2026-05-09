#pragma once
#include <stdint.h>

/**
 * @brief Закрашивает прямоугольник заданным образцом по заданным координатам в видеопамяти
 */
void title_sp_paint_brick_long(uint16_t x, uint16_t y, uint16_t x_width, uint16_t y_width, uint8_t color);
void title_sp_4_15_put(uint16_t x, uint16_t y, const uint8_t *image);
void title_sp_put(uint16_t x, uint16_t y, uint16_t x_width, uint16_t y_width, const uint8_t *image, const uint8_t *outline);
