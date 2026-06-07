#include "digger_music_title.h"

// Основная музыка "Popcorn"
const uint8_t popcorn_periods[] = {
        D4, C4,
        D4, A3, F3, A3, D3, D4, C4,
        D4, A3, F3, A3, D3, D4, E4,
        F4, E4, F4, D4, E4, D4, E4, C4,
        D4, C4, D4, AS3, D4, D4, C4,

        D4, A3, F3, A3, D3, D4, C4,
        D4, A3, F3, A3, D3, D4, E4,
        F4, E4, F4, D4, E4, D4, E4, C4,
        D4, C4, D4, E4, F4, A4, G4,

        A4, F4, C4, F4, A3, A4, G4,
        A4, F4, C4, F4, A3, A4, B4,
        C5, B4, C5, A4, B4, A4, B4, G4,
        A4, G4, A4, F4, A4, A4, G4,

        A4, F4, C4, F4, A3, A4, G4,
        A4, F4, C4, F4, A3, A4, B4,
        C5, B4, C5, A4, B4, A4, B4, G4,
        A4, G4, A4, F4, A4, D5, C5,

        A4, F4, C4, F4, A3, A4, G4,
        A4, F4, C4, F4, A3, A4, B4,
        C5, B4, C5, A4, B4, A4, B4, G4,
        A4, G4, F4, G4, A4,

        0
};

const uint8_t popcorn_durations[] = {
        NE, NE,
        NE, NE, NE, NE, NQ, NE, NE,
        NE, NE, NE, NE, NQ, NE, NE,
        NE, NE, NE, NE, NE, NE, NE, NE,
        NE, NE, NE, NE, NQ, NE, NE,

        NE, NE, NE, NE, NQ, NE, NE,
        NE, NE, NE, NE, NQ, NE, NE,
        NE, NE, NE, NE, NE, NE, NE, NE,
        NE, NE, NE, NE, NQ, NE, NE,

        NE, NE, NE, NE, NQ, NE, NE,
        NE, NE, NE, NE, NQ, NE, NE,
        NE, NE, NE, NE, NE, NE, NE, NE,
        NE, NE, NE, NE, NQ, NE, NE,

        NE, NE, NE, NE, NQ, NE, NE,
        NE, NE, NE, NE, NQ, NE, NE,
        NE, NE, NE, NE, NE, NE, NE, NE,
        NE, NE, NE, NE, NQ, NE, NE,

        NE, NE, NE, NE, NQ, NE, NE,
        NE, NE, NE, NE, NQ, NE, NE,
        NE, NE, NE, NE, NE, NE, NE, NE,
        NE, NE, NE, NE, NQ,

        0
};

// Музыка для режима "Бонус" - не используется в текущей сборке титульного
// экрана. Сами таблицы удалены, чтобы освободить ~322 байта data-сегмента
// для большой ZX0-обложки. Объявления `extern` в digger_music_title.h
// тоже убраны.
