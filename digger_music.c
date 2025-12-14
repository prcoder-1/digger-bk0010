#include "digger_music.h"

// Основная музыка "Popcorn"
const uint16_t popcorn_periods[] = {
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

const uint16_t popcorn_durations[] = {
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

 // Музыка для режима "Бонус"
const uint16_t bonus_periods[] = {
    C4, C4, C4, C4, C4, C4, C4, C4, F4, G4, A4,
    C4, C4, C4, C4, C4, C4, F4, A4, G4, E4, C4,
    C4, C4, C4, C4, C4, C4, C4, C4, F4, G4, A4,
    F4, A4, C5, AS4, A4, G4, F4, A4, F4,

    C4, C4, C4, C4, C4, C4, C4, C4, F4, G4, A4,
    C4, C4, C4, C4, C4, C4, F4, A4, G4, E4, C4,
    C4, C4, C4, C4, C4, C4, C4, C4, F4, G4, A4,
    F4, A4, C5, AS4, A4, G4, F4, A4, F4,

    A4, A4, A4, A4, A4, A4, A4, A4, A4,
    D5, A4, D5, A4, D5, A4, G4, F4, E4, D4,
    A4, A4, A4, A4, A4, A4, A4, A4, A4,
    D5, A4, D5, A4, D5, C5, B4, C5, B4, C5,

    A4, A4, A4, A4, A4, A4, A4, A4, A4,
    D5, A4, D5, A4, D5, A4, G4, F4, E4, D4,
    A4, A4, A4, A4, A4, A4, A4, A4, A4,
    D5, A4, D5, A4, D5, C5, B4, C5, B4, C5,

    0
};

const uint16_t bonus_durations[] = {
    NE, NE, NQ, NE, NE, NQ, NE, NE, NQ, NQ, NQ,
    NE, NE, NQ, NE, NE, NQ, NE, NE, NQ, NQ, NQ,
    NE, NE, NQ, NE, NE, NQ, NE, NE, NQ, NQ, NQ,
    NE, NE, N10, NE, NE, NE, NQ, NQ, NQ,

    NE, NE, NQ, NE, NE, NQ, NE, NE, NQ, NQ, NQ,
    NE, NE, NQ, NE, NE, NQ, NE, NE, NQ, NQ, NQ,
    NE, NE, NQ, NE, NE, NQ, NE, NE, NQ, NQ, NQ,
    NE, NE, N10, NE, NE, NE, NQ, NQ, NQ,

    NE, NE, NQ, NE, NE, NQ, NE, NE, NQ,
    NQ, NQ, NQ, NQ, NQ, NQ, NQ, NQ, NQ, NQ,
    NE, NE, NQ, NE, NE, NQ, NE, NE, NQ,
    NQ, NQ, NQ, NQ, NQ, NQ, NQ, NQ, NQ, NQ,

    NE, NE, NQ, NE, NE, NQ, NE, NE, NQ,
    NQ, NQ, NQ, NQ, NQ, NQ, NQ, NQ, NQ, NQ,
    NE, NE, NQ, NE, NE, NQ, NE, NE, NQ,
    NQ, NQ, NQ, NQ, NQ, NQ, NQ, NQ, NQ, NQ,

    0
};
