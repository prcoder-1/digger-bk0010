#pragma once
#include <stdint.h>
#include "sound.h"

enum bg_music : uint16_t
{
    BG_MUSIC_POPCORN = 0u,  // Обычная фоновая музыка "Popcorn"
    BG_MUSIC_BONUS          // Музыка режима "Бонус"
};

uint16_t sound_mode_next(uint16_t mode); // Следующий режим по кругу: пищалка, AY (на БК 0011М), выключено

void bg_music_track(enum bg_music music); // Выбрать мелодию фоновой музыки и начать её с начала
extern uint16_t music_mode; // Режим фоновой музыки (enum sound_mode)
void bg_music_frame(); // Начало кадра: продвинуть мелодию на AY
void bg_music_play(); // Воспроизвести фоновую музыку пищалкой в свободном времени кадра
void bg_music_mute(); // Заглушить AY перед блокирующими сценами (пауза, гибель, конец уровня)
