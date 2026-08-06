#include "memory.h"
#include "digger_music_background.h"
#include "digger_music.h"
#include "sound.h"

// Темп фоновой музыки. Единица времени — "слот" (один кусочек ноты); за кадр
// их выдаётся ровно music_chunks_per_frame, поэтому 1/8 нота = music_slots_ne /
// music_chunks_per_frame = 2 кадра = 181.6 мс (165 BPM, в оригинале 178.5 мс).
constexpr uint16_t music_slots_ne = 8u;         // Слотов в 1/8 ноте
constexpr uint16_t music_chunks_per_frame = 4u; // Слотов музыки за кадр — задаёт темп
constexpr uint16_t music_chunk_ref = 1400u;  // Размер кусочка = music_chunk_ref/period полупериодов
constexpr uint16_t music_chunk_counts = 60u; // Ниже music_chunk_counts отсчётов кадра новый кусочек не начинаем.

// Состояние проигрывателя фоновой музыки
struct {
    uint16_t pos;     /// Индекс текущей ноты в мелодии
    uint16_t period;  /// Период текущей ноты
    uint16_t chunk;   /// Полупериодов в одном кусочке текущей ноты (постоянное реальное время)
    uint16_t left;    /// Слотов осталось в текущей фазе (тон / пауза)
    uint16_t eighth;  /// 1/8 длительности ноты в слотах - пауза после тона (0 = паузы не будет)
    uint8_t  silent;  /// Текущая фаза беззвучная (пауза между нотами или нота-пауза)
} mus;

// Периоды нот мелодии Popcorn
static const uint16_t popcorn_period[] = { D4, C4, A3, F3, D3, E4, F4, A4, G4, B4, C5, AS3, D5 };

// Размер кусочка (в полупериодах) для каждой ноты: music_chunk_ref/period, но не меньше одного полного цикла.
#define PCHUNK(p) ((uint8_t)((music_chunk_ref / (p)) < 2 ? 2 : (music_chunk_ref / (p))))

// Размер кусочка (в полупериодах) для каждой ноты
static const uint8_t popcorn_chunk[] = {
    PCHUNK(D4), PCHUNK(C4), PCHUNK(A3), PCHUNK(F3), PCHUNK(D3), PCHUNK(E4), PCHUNK(F4),
    PCHUNK(A4), PCHUNK(G4), PCHUNK(B4), PCHUNK(C5), PCHUNK(AS3), PCHUNK(D5)
};

// Индексы нот мелодии Popcorn: 0..12 - восьмая, +13 - четвертная.
static const uint8_t popcorn_notes[] = {
    0, 1,

    0, 2, 3, 2, 17, 0, 1,
    0, 2, 3, 2, 17, 0, 5,
    6, 5, 6, 0, 5, 0, 5, 1,
    0, 1, 0, 11, 13, 0, 1,

    0, 2, 3, 2, 17, 0, 1,
    0, 2, 3, 2, 17, 0, 5,
    6, 5, 6, 0, 5, 0, 5, 1,
    0, 1, 0, 5, 19, 7, 8,

    7, 6, 1, 6, 15, 7, 8,
    7, 6, 1, 6, 15, 7, 9,
    10, 9, 10, 7, 9, 7, 9, 8,
    7, 8, 7, 6, 20, 7, 8,

    7, 6, 1, 6, 15, 7, 8,
    7, 6, 1, 6, 15, 7, 9,
    10, 9, 10, 7, 9, 7, 9, 8,
    7, 8, 7, 6, 20, 12, 10,

    7, 6, 1, 6, 15, 7, 8,
    7, 6, 1, 6, 15, 7, 9,
    10, 9, 10, 7, 9, 7, 9, 8,
    7, 8, 6, 8, 20
};

// Инициализация фоновой музыки
void bg_music_init(void)
{
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
            // Следующая нота (нот-пауз в мелодии нет - как и в оригинале)
            uint8_t n = popcorn_notes[mus.pos];
            if (++mus.pos >= sizeof(popcorn_notes)) mus.pos = 0;

            uint16_t slots = music_slots_ne;
            if (n >= 13) { n -= 13; slots <<= 1; } // длинная (NQ) нота — вдвое дольше
            mus.period = popcorn_period[n];
            mus.chunk = popcorn_chunk[n];

            mus.eighth = slots >> 3;       // пауза 1/8 после тона
            mus.left = slots - mus.eighth; // тон 7/8 длительности
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
