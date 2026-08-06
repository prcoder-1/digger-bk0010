#pragma once
#include <stdint.h>

enum bg_music : uint16_t
{
    BG_MUSIC_POPCORN = 0u,  // Обычная фоновая музыка "Popcorn"
    BG_MUSIC_BONUS          // Музыка режима "Бонус"
};

void bg_music_track(enum bg_music music); // Выбрать мелодию фоновой музыки и начать её с начала
void bg_music_play(); // Воспроизвести фоновую музыку в свободном времени кадра
