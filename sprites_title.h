#pragma once
#include <stdint.h>

/**
 * @brief Закрашивает прямоугольник заданным образцом по заданным координатам в видеопамяти
 */
void title_sp_paint_brick_long(uint16_t x, uint16_t y, uint16_t x_width, uint16_t y_width, uint8_t color);
void title_sp_4_15_put(uint16_t x, uint16_t y, const uint8_t *image);
void title_sp_put(uint16_t x, uint16_t y, uint16_t x_width, uint16_t y_width, const uint8_t *image, const uint8_t *outline);

/**
 * @brief Очистка вертикальной полосы шириной 1 байт (для стирания следа спрайта).
 * Тонкий специализированный путь без push/pop вокруг music_tick — поллинг строго раз в строку.
 */
void title_sp_clear_strip(uint16_t x, uint16_t y, uint16_t height);
