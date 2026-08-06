#include "memory.h"
#include "digger_music_background.h"
#include "digger_music.h"
#include "sound.h"

// Темп фоновой музыки. Единица времени — "слот" (один кусочек ноты); за кадр
// их выдаётся ровно music_chunks_per_frame, поэтому 1/8 нота Popcorn =
// popcorn_slots_ne / music_chunks_per_frame = 2 кадра = 181.6 мс (165 BPM, в
// оригинале 178.5 мс). Бонус-мелодия в оригинале ровно вдвое быстрее, поэтому
// её 1/8 = 1 кадр = 90.8 мс (в оригинале 89.25 мс).
constexpr uint16_t popcorn_slots_ne = 8u;       // Слотов в 1/8 ноте мелодии Popcorn
constexpr uint16_t bonus_slots_ne = 4u;         // Слотов в 1/8 ноте бонус-мелодии
constexpr uint8_t  note_kinds = 14u;            // Число различных высот в общей таблице
constexpr uint16_t music_chunks_per_frame = 4u; // Слотов музыки за кадр — задаёт темп
constexpr uint16_t music_chunk_ref = 1400u;     // Размер кусочка = music_chunk_ref/period полупериодов
constexpr uint16_t music_chunk_counts = 60u;    // Ниже music_chunk_counts отсчётов кадра новый кусочек не начинаем.

// Состояние проигрывателя фоновой музыки
struct {
    const uint8_t *notes; /// Ноты текущей мелодии
    uint16_t count;       /// Количество нот в текущей мелодии
    uint16_t slots_ne;    /// Слотов в 1/8 ноте текущей мелодии (задаёт темп)
    uint16_t slots_n10;   /// Слотов в ноте длиной 5 восьмых (готовое значение - чтобы не звать __mulhi3)
    uint16_t pos;         /// Индекс текущей ноты в мелодии
    uint16_t period;      /// Период текущей ноты
    uint16_t chunk;       /// Полупериодов в одном кусочке текущей ноты (постоянное реальное время)
    uint16_t left;        /// Слотов осталось в текущей фазе (тон / пауза)
    uint16_t eighth;      /// 1/8 длительности ноты в слотах - пауза после тона (0 = паузы не будет)
    uint8_t  silent;      /// Текущая фаза беззвучная (пауза между нотами или нота-пауза)
} mus;

// Периоды нот, общие для обеих мелодий. Индексы 0..12 - Popcorn, 13 (AS4) - только бонус.
static const uint16_t note_period[] = { D4, C4, A3, F3, D3, E4, F4, A4, G4, B4, C5, AS3, D5, AS4 };

// Размер кусочка (в полупериодах) для каждой ноты: music_chunk_ref/period, но не меньше одного полного цикла.
#define PCHUNK(p) ((uint8_t)((music_chunk_ref / (p)) < 2 ? 2 : (music_chunk_ref / (p))))

// Размер кусочка (в полупериодах) для каждой ноты
static const uint8_t note_chunk[] = {
    PCHUNK(D4), PCHUNK(C4), PCHUNK(A3), PCHUNK(F3), PCHUNK(D3), PCHUNK(E4), PCHUNK(F4),
    PCHUNK(A4), PCHUNK(G4), PCHUNK(B4), PCHUNK(C5), PCHUNK(AS3), PCHUNK(D5), PCHUNK(AS4)
};

// Индексы нот мелодии "Popcorn": 0..13 - восьмая, +14 - четвертная, +28 - 5 восьмых.
static const uint8_t popcorn_notes[] = {
    0, 1,

    0, 2, 3, 2, 18, 0, 1,
    0, 2, 3, 2, 18, 0, 5,
    6, 5, 6, 0, 5, 0, 5, 1,
    0, 1, 0, 11, 14, 0, 1,

    0, 2, 3, 2, 18, 0, 1,
    0, 2, 3, 2, 18, 0, 5,
    6, 5, 6, 0, 5, 0, 5, 1,
    0, 1, 0, 5, 20, 7, 8,

    7, 6, 1, 6, 16, 7, 8,
    7, 6, 1, 6, 16, 7, 9,
    10, 9, 10, 7, 9, 7, 9, 8,
    7, 8, 7, 6, 21, 7, 8,

    7, 6, 1, 6, 16, 7, 8,
    7, 6, 1, 6, 16, 7, 9,
    10, 9, 10, 7, 9, 7, 9, 8,
    7, 8, 7, 6, 21, 12, 10,

    7, 6, 1, 6, 16, 7, 8,
    7, 6, 1, 6, 16, 7, 9,
    10, 9, 10, 7, 9, 7, 9, 8,
    7, 8, 6, 8, 21
};

// Индексы нот мелодии режима "Бонус" (галоп из увертюры "Вильгельм Телль")
static const uint8_t bonus_notes[] = {
    // A
    1, 1, 15, 1, 1, 15, 1, 1, 20, 22, 21,
    1, 1, 15, 1, 1, 15, 6, 7, 22, 19, 15,
    1, 1, 15, 1, 1, 15, 1, 1, 20, 22, 21,
    6, 7, 38, 13, 7, 8, 20, 21, 20,
    // A
    1, 1, 15, 1, 1, 15, 1, 1, 20, 22, 21,
    1, 1, 15, 1, 1, 15, 6, 7, 22, 19, 15,
    1, 1, 15, 1, 1, 15, 1, 1, 20, 22, 21,
    6, 7, 38, 13, 7, 8, 20, 21, 20,
    // B
    7, 7, 21, 7, 7, 21, 7, 7, 21,
    26, 21, 26, 21, 26, 21, 22, 20, 19, 14,
    7, 7, 21, 7, 7, 21, 7, 7, 21,
    26, 21, 26, 21, 26, 24, 23, 24, 23, 24,
    // B
    7, 7, 21, 7, 7, 21, 7, 7, 21,
    26, 21, 26, 21, 26, 21, 22, 20, 19, 14,
    7, 7, 21, 7, 7, 21, 7, 7, 21,
    26, 21, 26, 21, 26, 24, 23, 24, 23, 24
};

// Выбрать мелодию фоновой музыки и начать её с начала
void bg_music_track(enum bg_music music)
{
    switch (music)
    {
        case BG_MUSIC_POPCORN:
        {
            mus.notes = popcorn_notes;
            mus.count = sizeof(popcorn_notes);
            mus.slots_ne = popcorn_slots_ne;
            mus.slots_n10 = 5u * popcorn_slots_ne;
            break;
        }

        case BG_MUSIC_BONUS:
        {
            mus.notes = bonus_notes;
            mus.count = sizeof(bonus_notes);
            mus.slots_ne = bonus_slots_ne;
            mus.slots_n10 = 5u * bonus_slots_ne;
            break;
        }
    }

    mus.pos = 0;
    mus.left = 0;
    mus.eighth = 0;
}

// Проиграть один слот (кусочек) текущей ноты фоновой музыки
void bg_music_tick(void)
{
    if (mus.left == 0) // текущая фаза доиграна
    {
        if (mus.eighth) // тон доиграл - пауза 1/8 длительности перед следующей нотой
        {
            mus.left = mus.eighth;
            mus.eighth = 0;
            mus.silent = 1;
        }
        else
        {
            // Следующая нота (нот-пауз в мелодиях нет - как и в оригинале)
            uint8_t n = mus.notes[mus.pos];
            if (++mus.pos >= mus.count) mus.pos = 0;

            uint16_t slots;
            if (n >= 2 * note_kinds)
            {
                n -= 2 * note_kinds;
                slots = mus.slots_n10;        // нота в 5 восьмых
            }
            else if (n >= note_kinds)
            {
                n -= note_kinds;
                slots = mus.slots_ne << 1;    // четвертная нота - вдвое дольше
            }
            else
            {
                slots = mus.slots_ne;         // восьмая нота
            }
            mus.period = note_period[n];
            mus.chunk = note_chunk[n];

            // Пауза ~1/8 длительности, но не меньше слота (у бонуса 1/8 нота = 4 слота)
            mus.eighth = (slots + 7) >> 3;
            mus.left = slots - mus.eighth; // тон - остаток длительности
            mus.silent = 0;
        }
    }

    if (!mus.silent) sound_pwm(mus.period, mus.chunk, 1);
    --mus.left;
}

// Воспроизвести фоновую музыку в свободном времени кадра
void bg_music_play()
{
        volatile uint16_t *t_count = (volatile uint16_t *)REG_TVE_COUNT;
        volatile union TVE_CSR *tve_csr = (volatile union TVE_CSR *)REG_TVE_CSR;

        uint16_t music_budget = music_chunks_per_frame;
        while (music_budget && *t_count > music_chunk_counts && (tve_csr->reg & (1 << TVE_CSR_FL)) == 0)
        {
            bg_music_tick();
            --music_budget;
        }
}
