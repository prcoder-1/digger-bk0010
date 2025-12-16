#pragma once
#include <stdint.h>

/**
 * @brief Вывод звука (прямоугольными импульсами)
 *
 * @param period - период звука (1 / частота)
 * @param durance - длительность звука
 */
void sound(uint16_t period, uint16_t durance);

/**
 * @brief Вывод вибрирующего звука
 *
 * @param period - период звука (1 / частота)
 * @param durance - длительность звука
 */
void sound_vibrato(uint16_t period, uint8_t durance);
