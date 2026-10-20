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

#define AY_CLOCK 1714286.0 // Такт AY, Гц (12 МГц / 7, «Мультизвук»)

// Период тона AY для ноты с периодом p пищалки: та же частота, что у sound_pwm на
// БК 0010 (полупериод 20/3 * p + 25.3 мкс - замер в эмуляторе)
#define AYP(p) ((uint16_t)(AY_CLOCK * (40.0 * (p) / 3.0 + 50.6) / 16e6 + 0.5))

enum : uint8_t { AY_TONE_A_LO = 0, AY_TONE_A_HI = 1, AY_VOL_A = 8, AY_ENV_HI = 12, AY_REGS = 14 };

// Регистры AY, пишутся целиком в начале ноты. Тон только в канале A, громкость от
// огибающей «спад до нуля» (13 = 011, запись формы её перезапускает). Период огибающей -
// слоты ноты * 256: спад длится ~1.7 длительности ноты.
static uint8_t ay_regs[AY_REGS] = { [7] = 076, [AY_VOL_A] = 020, [13] = 011 };

static enum bg_music_mode mode = BG_MUSIC_BEEPER;

// Состояние проигрывателя фоновой музыки
struct {
    const uint8_t (*parts)[2];     /// Куски текущей мелодии
    const uint8_t (*parts_end)[2]; /// Конец списка кусков
    const uint8_t (*part)[2];      /// Следующий кусок
    const uint8_t *note;           /// Следующая нота текущего куска
    uint16_t part_left;            /// Нот осталось в текущем куске
    uint16_t slots_ne;    /// Слотов в 1/8 ноте текущей мелодии (задаёт темп)
    uint16_t slots_n10;   /// Слотов в ноте длиной 5 восьмых (готовое значение - чтобы не звать __mulhi3)
    uint16_t period;      /// Период текущей ноты
    uint16_t chunk;       /// Полупериодов в одном кусочке текущей ноты (постоянное реальное время)
    uint16_t left;        /// Слотов осталось в текущей фазе (тон / пауза)
    uint16_t eighth;      /// 1/8 длительности ноты в слотах - пауза после тона (0 = паузы не будет)
    uint8_t  silent;      /// Текущая фаза беззвучная (пауза между нотами или нота-пауза)
} mus;

// Периоды нот, общие для обеих мелодий. Индексы 0..12 - Popcorn, 13 (AS4) - только бонус.
static const uint16_t note_period[] = { D4, C4, A3, F3, D3, E4, F4, A4, G4, B4, C5, AS3, D5, AS4 };

// Периоды тона AY для тех же нот
static const uint16_t note_ay_period[] = {
    AYP(D4), AYP(C4), AYP(A3), AYP(F3), AYP(D3), AYP(E4), AYP(F4),
    AYP(A4), AYP(G4), AYP(B4), AYP(C5), AYP(AS3), AYP(D5), AYP(AS4)
};

// Размер кусочка (в полупериодах) для каждой ноты: music_chunk_ref/period, но не меньше одного полного цикла.
#define PCHUNK(p) ((uint8_t)((music_chunk_ref / (p)) < 2 ? 2 : (music_chunk_ref / (p))))

// Размер кусочка (в полупериодах) для каждой ноты
static const uint8_t note_chunk[] = {
    PCHUNK(D4), PCHUNK(C4), PCHUNK(A3), PCHUNK(F3), PCHUNK(D3), PCHUNK(E4), PCHUNK(F4),
    PCHUNK(A4), PCHUNK(G4), PCHUNK(B4), PCHUNK(C5), PCHUNK(AS3), PCHUNK(D5), PCHUNK(AS4)
};

// Ноты мелодий: 0..13 - восьмая, +14 - четвертная, +28 - 5 восьмых. Повторяющиеся
// куски хранятся один раз, мелодия - список кусков (смещение в notes_pool, длина).
enum : uint8_t
{
    P_INTRO = 0, P_S1 = P_INTRO + 2, P_E1 = P_S1 + 22, P_E2 = P_E1 + 7, P_S2 = P_E2 + 7,
    P_F1 = P_S2 + 22, P_F2 = P_F1 + 7, P_F3 = P_F2 + 7, B_A = P_F3 + 5, B_B = B_A + 42
};

static const uint8_t notes_pool[] = {
    // "Popcorn"
    0, 1,
    0, 2, 3, 2, 18, 0, 1, 0, 2, 3, 2, 18, 0, 5, 6, 5, 6, 0, 5, 0, 5, 1,
    0, 1, 0, 11, 14, 0, 1,
    0, 1, 0, 5, 20, 7, 8,
    7, 6, 1, 6, 16, 7, 8, 7, 6, 1, 6, 16, 7, 9, 10, 9, 10, 7, 9, 7, 9, 8,
    7, 8, 7, 6, 21, 7, 8,
    7, 8, 7, 6, 21, 12, 10,
    7, 8, 6, 8, 21,
    // Режим "Бонус" (галоп из увертюры "Вильгельм Телль")
    1, 1, 15, 1, 1, 15, 1, 1, 20, 22, 21, 1, 1, 15, 1, 1, 15, 6, 7, 22, 19, 15, 1, 1, 15, 1, 1, 15, 1, 1, 20, 22, 21, 6, 7, 38, 13, 7, 8, 20, 21, 20,
    7, 7, 21, 7, 7, 21, 7, 7, 21, 26, 21, 26, 21, 26, 21, 22, 20, 19, 14, 7, 7, 21, 7, 7, 21, 7, 7, 21, 26, 21, 26, 21, 26, 24, 23, 24, 23, 24
};

static const uint8_t popcorn_parts[][2] = {
    { P_INTRO, 2 }, { P_S1, 22 }, { P_E1, 7 }, { P_S1, 22 }, { P_E2, 7 },
    { P_S2, 22 }, { P_F1, 7 }, { P_S2, 22 }, { P_F2, 7 }, { P_S2, 22 }, { P_F3, 5 }
};

static const uint8_t bonus_parts[][2] = { { B_A, 42 }, { B_A, 42 }, { B_B, 38 }, { B_B, 38 } };

// Выбрать мелодию фоновой музыки и начать её с начала
void bg_music_track(enum bg_music music)
{
    switch (music)
    {
        case BG_MUSIC_POPCORN:
        {
            mus.parts = popcorn_parts;
            mus.parts_end = popcorn_parts + sizeof(popcorn_parts) / 2;
            mus.slots_ne = popcorn_slots_ne;
            mus.slots_n10 = 5u * popcorn_slots_ne;
            break;
        }

        case BG_MUSIC_BONUS:
        {
            mus.parts = bonus_parts;
            mus.parts_end = bonus_parts + sizeof(bonus_parts) / 2;
            mus.slots_ne = bonus_slots_ne;
            mus.slots_n10 = 5u * bonus_slots_ne;
            break;
        }
    }

    mus.part = mus.parts;
    mus.part_left = 0;
    mus.left = 0;
    mus.eighth = 0;
}

// Перейти к следующему слоту мелодии. Возвращает не 0, если в нём началась нота
static uint16_t bg_music_advance(void)
{
    uint16_t started = 0;
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
            if (!mus.part_left)
            {
                if (mus.part == mus.parts_end) mus.part = mus.parts;
                mus.note = notes_pool + (*mus.part)[0];
                mus.part_left = (*mus.part)[1];
                ++mus.part;
            }
            --mus.part_left;
            uint8_t n = *mus.note++;

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
            const uint16_t ay_period = note_ay_period[n];
            ay_regs[AY_TONE_A_LO] = (uint8_t)ay_period;
            ay_regs[AY_TONE_A_HI] = (uint8_t)(ay_period >> 8);
            ay_regs[AY_ENV_HI] = (uint8_t)slots;
            started = 1;

            // Пауза ~1/8 длительности, но не меньше слота (у бонуса 1/8 нота = 4 слота)
            mus.eighth = (slots + 7) >> 3;
            mus.left = slots - mus.eighth; // тон - остаток длительности
            mus.silent = 0;
        }
    }

    --mus.left;
    return started;
}

// Запись в регистр AY: словом - номер регистра, байтом - данные; линии порта инверсные
static void ay_write(uint8_t reg, uint8_t value)
{
    volatile uint16_t *port = (volatile uint16_t *)REG_PAR_INTERF;
    *port = (uint16_t)~reg;
    *(volatile uint8_t *)port = (uint8_t)~value;
}

void bg_music_mute()
{
    if (mode == BG_MUSIC_AY) ay_write(AY_VOL_A, 0);
}

void bg_music_next_mode()
{
    bg_music_mute();
    if (++mode == BG_MUSIC_AY && !bk_11m) ++mode;
    if (mode > BG_MUSIC_OFF) mode = BG_MUSIC_BEEPER;
}

// Пауза перед нотой не нужна: ноты разделяет перезапуск огибающей. Ноты начинаются
// на границе кадра (длительности кратны четырём слотам), поэтому шаг - целый кадр.
void bg_music_frame()
{
    if (mode != BG_MUSIC_AY) return;

    uint16_t started = 0;
    for (uint16_t i = music_chunks_per_frame; i; --i) started |= bg_music_advance();

    if (started) for (uint16_t r = 0; r < AY_REGS; ++r) ay_write(r, ay_regs[r]);
    else ay_write(AY_VOL_A, 020); // Снять bg_music_mute
}

// Воспроизвести фоновую музыку в свободном времени кадра
void bg_music_play()
{
        if (mode != BG_MUSIC_BEEPER) return;

        volatile uint16_t *t_count = (volatile uint16_t *)REG_TVE_COUNT;
        volatile union TVE_CSR *tve_csr = (volatile union TVE_CSR *)REG_TVE_CSR;

        uint16_t music_budget = music_chunks_per_frame;
        // Порог в реальном времени: на БК 0011М таймер считает быстрее
        const uint16_t chunk_counts = bk_11m ? music_chunk_counts * TVE_FREQ_11M / TVE_FREQ_0010 : music_chunk_counts;
        while (music_budget && *t_count > chunk_counts && (tve_csr->reg & (1 << TVE_CSR_FL)) == 0)
        {
            bg_music_advance();
            if (!mus.silent) sound_pwm(mus.period, mus.chunk, 1);
            --music_budget;
        }
}
