#pragma once
#include <stdint.h>

// Режим звука (музыки и эффектов)
enum sound_mode : uint16_t
{
    SOUND_BEEPER = 0u, // Пищалка
    SOUND_AY,          // AY-3-8910 на порту 0177714 (только БК 0011М)
    SOUND_OFF          // Выключено
};

extern uint16_t fx_mode; // Режим sound()/sound_vibrato() - звуковых эффектов игры

// Эффект на AY: тон с частотой пищалки для периода period; 0 - вернуть канал музыке.
// Реализует игра (digger_music_background.c).
void ay_fx(uint16_t period);

/**
 * @brief Вывод звука (прямоугольными импульсами). В режиме fx_mode == SOUND_AY звучит
 *        тон AY, а время выдерживает тот же цикл с выключенной пищалкой
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
 * вызывающий код обязан гарантировать 1 <= pw <= 2*period - 1 (с учётом
 * пересчёта под БК 0011М — 1 <= pw <= period).
 *
 * durance - число полупериодов (как у sound_vibrato).
 *
 * @param period - период звука (1 / частота)
 * @param durance - длительность звука
 * @param pw     - ширина «ON»-фазы в sob-тактах
 */
void sound_pwm(uint16_t period, uint16_t durance, uint16_t pw);
