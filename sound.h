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
void sound_vibrato(uint16_t period, uint16_t durance);

/**
 * @brief Вывод тона с заданной шириной импульса (PWM, для огибающей).
 *
 * Один аудио-цикл = 2*period sob-тактов: динамик ON на pw, OFF на
 * (2*period - pw). Громкость пропорциональна pw/(2*period); pw=period
 * даёт максимум (50% duty). Чтобы внутренние sob-петли не зациклились,
 * вызывающий код обязан гарантировать 1 <= pw <= 2*period - 1.
 *
 * durance - число полупериодов (как у sound_vibrato).
 *
 * @param period - период звука (1 / частота)
 * @param durance - длительность звука
 * @param pw     - ширина «ON»-фазы в sob-тактах
 */
void sound_pwm(uint16_t period, uint16_t durance, uint16_t pw);
