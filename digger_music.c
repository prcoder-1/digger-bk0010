#include "digger_music.h"

// Основная музыка "Popcorn"
const uint16_t popcorn_periods[] = {
        D4 / NV, C4 / NV,
        D4 / NV, A3 / NV, F3 / NV, A3 / NV, D3 / NV, D4 / NV, C4 / NV,
        D4 / NV, A3 / NV, F3 / NV, A3 / NV, D3 / NV, D4 / NV, E4 / NV,
        F4 / NV, E4 / NV, F4 / NV, D4 / NV, E4 / NV, D4 / NV, E4 / NV, C4 / NV,
        D4 / NV, C4 / NV, D4 / NV, AS3 / NV, D4 / NV, D4 / NV, C4 / NV,

        D4 / NV, A3 / NV, F3 / NV, A3 / NV, D3 / NV, D4 / NV, C4 / NV,
        D4 / NV, A3 / NV, F3 / NV, A3 / NV, D3 / NV, D4 / NV, E4 / NV,
        F4 / NV, E4 / NV, F4 / NV, D4 / NV, E4 / NV, D4 / NV, E4 / NV, C4 / NV,
        D4 / NV, C4 / NV, D4 / NV, E4 / NV, F4 / NV, A4 / NV, G4 / NV,

        A4 / NV, F4 / NV, C4 / NV, F4 / NV, A3 / NV, A4 / NV, G4 / NV,
        A4 / NV, F4 / NV, C4 / NV, F4 / NV, A3 / NV, A4 / NV, B4 / NV,
        C5 / NV, B4 / NV, C5 / NV, A4 / NV, B4 / NV, A4 / NV, B4 / NV, G4 / NV,
        A4 / NV, G4 / NV, A4 / NV, F4 / NV, A4 / NV, A4 / NV, G4 / NV,

        A4 / NV, F4 / NV, C4 / NV, F4 / NV, A3 / NV, A4 / NV, G4 / NV,
        A4 / NV, F4 / NV, C4 / NV, F4 / NV, A3 / NV, A4 / NV, B4 / NV,
        C5 / NV, B4 / NV, C5 / NV, A4 / NV, B4 / NV, A4 / NV, B4 / NV, G4 / NV,
        A4 / NV, G4 / NV, A4 / NV, F4 / NV, A4 / NV, D5 / NV, C5 / NV,

        A4 / NV, F4 / NV, C4 / NV, F4 / NV, A3 / NV, A4 / NV, G4 / NV,
        A4 / NV, F4 / NV, C4 / NV, F4 / NV, A3 / NV, A4 / NV, B4 / NV,
        C5 / NV, B4 / NV, C5 / NV, A4 / NV, B4 / NV, A4 / NV, B4 / NV, G4 / NV,
        A4 / NV, G4 / NV, F4 / NV, G4 / NV, A4 / NV,

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
    C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, F4 / NV, G4 / NV, A4 / NV,
    C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, F4 / NV, A4 / NV, G4 / NV, E4 / NV, C4 / NV,
    C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, F4 / NV, G4 / NV, A4 / NV,
    F4 / NV, A4 / NV, C5 / NV, AS4 / NV, A4 / NV, G4 / NV, F4 / NV, A4 / NV, F4 / NV,

    C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, F4 / NV, G4 / NV, A4 / NV,
    C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, F4 / NV, A4 / NV, G4 / NV, E4 / NV, C4 / NV,
    C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, C4 / NV, F4 / NV, G4 / NV, A4 / NV,
    F4 / NV, A4 / NV, C5 / NV, AS4 / NV, A4 / NV, G4 / NV, F4 / NV, A4 / NV, F4 / NV,

    A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV,
    D5 / NV, A4 / NV, D5 / NV, A4 / NV, D5 / NV, A4 / NV, G4 / NV, F4 / NV, E4 / NV, D4 / NV,
    A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV,
    D5 / NV, A4 / NV, D5 / NV, A4 / NV, D5 / NV, C5 / NV, B4 / NV, C5 / NV, B4 / NV, C5 / NV,

    A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV,
    D5 / NV, A4 / NV, D5 / NV, A4 / NV, D5 / NV, A4 / NV, G4 / NV, F4 / NV, E4 / NV, D4 / NV,
    A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV, A4 / NV,
    D5 / NV, A4 / NV, D5 / NV, A4 / NV, D5 / NV, C5 / NV, B4 / NV, C5 / NV, B4 / NV, C5 / NV,

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
