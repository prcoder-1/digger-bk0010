#pragma once
#include <stdint.h>

// Используем тот же набор макросов нот и длительностей, что и основная игра,
// чтобы popcorn_periods[] / popcorn_durations[] звучали через sound_vibrato
// с теми же музыкальными значениями, что и траурный марш в digger.c::man_rip().
//
// До этого `digger_music_title.h` хранил собственные значения, заточенные под
// polling-music_service (формула cycles_left = duration*1472, ноты сдвинуты
// на октаву вверх для влезания в uint8_t). После перехода на синхронный
// sound_vibrato эти подмены не нужны: durations типа NE=128/NQ=256 хорошо
// работают, а популярные ноты ниже B3 теперь спокойно укладываются в uint16_t.
#include "digger_music.h"

// Основная музыка "Popcorn"
// uint16_t: значения нот ниже B3 (D3=406, F3=341, A3=271) и длительности
// от NQ=256 и больше не помещаются в uint8_t.
extern const uint16_t popcorn_periods[];
extern const uint16_t popcorn_durations[];
