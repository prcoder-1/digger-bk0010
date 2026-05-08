#pragma once
#include <stdint.h>

/**
 * @brief Закрашивает прямоугольник заданным образцом по заданным координатам в видеопамяти
 */
void sp_paint_brick_long(uint16_t x, uint16_t y, uint16_t x_width, uint16_t y_width, uint8_t color);
