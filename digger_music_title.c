#include "digger_music_title.h"

// Основная музыка "Popcorn"
// Тип uint16_t: periods для нот ниже B3 (D3=406, F3=341, A3=271) не
// помещаются в uint8_t; durations типа NQ=256, NH=512 - тем более.
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

// Длительности popcorn-нот, выровненные по реальному времени звучания.
//
// sound_vibrato(period, durance) тратит ~ durance * period CPU-циклов
// (внутренний sob крутится period раз на каждой внешней итерации).
// Поэтому при одинаковом durance низкие ноты с большим period звучат
// дольше высоких. Чтобы NE/NQ соответствовали РЕАЛЬНОМУ времени, а не
// "количеству звуковых волн", приводим: durance = class * REF_P / period.
//
// Расчёт - в макросе DUR на этапе компиляции (gcc сворачивает константы):
// деление uint16_t/uint16_t в рантайме потребовало бы __udivhi3 из
// divmulmod.s (~682 байта, не помещается в свободные ~278 байт title-бинаря).
//
// REF_P подобран так, чтобы худший случай durance * REF_P (NQ*REF_P =
// 256*200 = 51200) ещё помещался в uint16_t, и при этом NE-нота
// получалась ~50-100 мс при типичных period 100-400.
#define REF_P 200u
#define DUR(c, p) ((uint16_t)((unsigned)(c) * REF_P / (p)))

const uint16_t popcorn_durations[] = {

        DUR(NE, D4), DUR(NE, C4),
        DUR(NE, D4), DUR(NE, A3), DUR(NE, F3), DUR(NE, A3), DUR(NQ, D3), DUR(NE, D4), DUR(NE, C4),
        DUR(NE, D4), DUR(NE, A3), DUR(NE, F3), DUR(NE, A3), DUR(NQ, D3), DUR(NE, D4), DUR(NE, E4),
        DUR(NE, F4), DUR(NE, E4), DUR(NE, F4), DUR(NE, D4), DUR(NE, E4), DUR(NE, D4), DUR(NE, E4), DUR(NE, C4),
        DUR(NE, D4), DUR(NE, C4), DUR(NE, D4), DUR(NE, AS3), DUR(NQ, D4), DUR(NE, D4), DUR(NE, C4),

        DUR(NE, D4), DUR(NE, A3), DUR(NE, F3), DUR(NE, A3), DUR(NQ, D3), DUR(NE, D4), DUR(NE, C4),
        DUR(NE, D4), DUR(NE, A3), DUR(NE, F3), DUR(NE, A3), DUR(NQ, D3), DUR(NE, D4), DUR(NE, E4),
        DUR(NE, F4), DUR(NE, E4), DUR(NE, F4), DUR(NE, D4), DUR(NE, E4), DUR(NE, D4), DUR(NE, E4), DUR(NE, C4),
        DUR(NE, D4), DUR(NE, C4), DUR(NE, D4), DUR(NE, E4), DUR(NQ, F4), DUR(NE, A4), DUR(NE, G4),

        DUR(NE, A4), DUR(NE, F4), DUR(NE, C4), DUR(NE, F4), DUR(NQ, A3), DUR(NE, A4), DUR(NE, G4),
        DUR(NE, A4), DUR(NE, F4), DUR(NE, C4), DUR(NE, F4), DUR(NQ, A3), DUR(NE, A4), DUR(NE, B4),
        DUR(NE, C5), DUR(NE, B4), DUR(NE, C5), DUR(NE, A4), DUR(NE, B4), DUR(NE, A4), DUR(NE, B4), DUR(NE, G4),
        DUR(NE, A4), DUR(NE, G4), DUR(NE, A4), DUR(NE, F4), DUR(NQ, A4), DUR(NE, A4), DUR(NE, G4),

        DUR(NE, A4), DUR(NE, F4), DUR(NE, C4), DUR(NE, F4), DUR(NQ, A3), DUR(NE, A4), DUR(NE, G4),
        DUR(NE, A4), DUR(NE, F4), DUR(NE, C4), DUR(NE, F4), DUR(NQ, A3), DUR(NE, A4), DUR(NE, B4),
        DUR(NE, C5), DUR(NE, B4), DUR(NE, C5), DUR(NE, A4), DUR(NE, B4), DUR(NE, A4), DUR(NE, B4), DUR(NE, G4),
        DUR(NE, A4), DUR(NE, G4), DUR(NE, A4), DUR(NE, F4), DUR(NQ, A4), DUR(NE, D5), DUR(NE, C5),

        DUR(NE, A4), DUR(NE, F4), DUR(NE, C4), DUR(NE, F4), DUR(NQ, A3), DUR(NE, A4), DUR(NE, G4),
        DUR(NE, A4), DUR(NE, F4), DUR(NE, C4), DUR(NE, F4), DUR(NQ, A3), DUR(NE, A4), DUR(NE, B4),
        DUR(NE, C5), DUR(NE, B4), DUR(NE, C5), DUR(NE, A4), DUR(NE, B4), DUR(NE, A4), DUR(NE, B4), DUR(NE, G4),
        DUR(NE, A4), DUR(NE, G4), DUR(NE, F4), DUR(NE, G4), DUR(NQ, A4),

        0

};

// Музыка для режима "Бонус" - не используется в текущей сборке титульного
// экрана. Сами таблицы удалены, чтобы освободить ~322 байта data-сегмента
// для большой ZX0-обложки. Объявления `extern` в digger_music_title.h
// тоже убраны.
