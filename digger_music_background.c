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

enum : uint8_t { AY_TONE_B_LO = 2, AY_VOL_B = 9, AY_ENV_HI = 12, AY_ENV_SHAPE = 13, AY_REGS = 13 };

// Регистры AY 0..12, пишутся целиком (ay_out); форма огибающей R13 - отдельно, в
// начале ноты. Тон только в канале B - при стерео ABC он посередине, в обоих каналах;
// его делят музыка и эффекты.
static uint8_t ay_regs[AY_REGS] __attribute__((aligned(2))) = { [7] = 075 };

uint16_t music_mode = SOUND_BEEPER;
static uint8_t music_vol; // Громкость музыки в канале B: 020 - от огибающей, 0 - пауза
static uint8_t music_gap; // Кадр кончается паузой между нотами - выдержать её в конце кадра

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
    uint16_t ay_period;   /// Период тона AY текущей ноты
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

// Мелодии (индекс - enum bg_music): куски, конец списка, слотов в 1/8 и в 5/8 ноты
static const struct {
    const uint8_t (*parts)[2], (*end)[2];
    uint16_t slots_ne, slots_n10;
} tracks[] = {
    { popcorn_parts, popcorn_parts + sizeof(popcorn_parts) / 2, popcorn_slots_ne, 5u * popcorn_slots_ne },
    { bonus_parts, bonus_parts + sizeof(bonus_parts) / 2, bonus_slots_ne, 5u * bonus_slots_ne }
};

// Период тона AY с частотой пищалки на БК 0010 для периода p: 10/7 * p + 5 (такт AY
// 1714286 Гц - «Мультизвук»; +5 - накладные команды цикла: у sound() полупериод
// 20/3 * p + 29.3 мкс, у sound_pwm 20/3 * p + 25.3 мкс - в обоих случаях ошибка ~0 центов)
__attribute__((noinline)) static uint16_t ay_tp(uint16_t p)
{
    return p + (p >> 1) - (p >> 4) - (p >> 7) + 5;
}

// Выбрать мелодию фоновой музыки и начать её с начала
void bg_music_track(enum bg_music music)
{
    mus.parts = mus.part = tracks[music].parts;
    mus.parts_end = tracks[music].end;
    mus.slots_ne = tracks[music].slots_ne;
    mus.slots_n10 = tracks[music].slots_n10;
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
            mus.ay_period = ay_tp(note_period[n]);
            ay_regs[AY_ENV_HI] = (uint8_t)slots; // Спад огибающей ~1.7 длительности ноты
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

// Запись в регистр AY на порту 0177714, как у INSULT: словом - номер регистра,
// байтом - данные; линии порта инверсные. В слове номера инвертирован только младший
// байт, старший - 0 (с 0377 там платы на два AY не выбирали ни один чип)
static void ay_write(uint8_t reg, uint8_t value)
{
    volatile uint16_t *port = (volatile uint16_t *)REG_PAR_INTERF;
    *port = (uint8_t)~reg;
    *(volatile uint8_t *)port = (uint8_t)~value;
}

// Записать регистры AY 0..12 с тоном и громкостью канала B
static void ay_out(uint16_t tp, uint8_t vol)
{
    *(uint16_t *)&ay_regs[AY_TONE_B_LO] = tp; // R2 и R3 одним словом
    ay_regs[AY_VOL_B] = vol;
    // Выбрать первый чип TurboSound (номер 255 в инверсии), как Prince of Persia
    // (CLR @#177714): второй мог остаться выбранным, и запись уходила в него
    volatile uint16_t *port = (volatile uint16_t *)REG_PAR_INTERF;
    const uint8_t *v = ay_regs;
    *port = 0;
    for (uint16_t r = 0377; r > 0377 - AY_REGS; --r) // Номер регистра в инверсии
    {
        *port = r;
        *(volatile uint8_t *)port = ~*v++;
    }
}

void ay_fx(uint16_t period)
{
    if (period) ay_out(ay_tp(period), 017);
    else ay_out(mus.ay_period, music_vol); // Вернуть канал музыке
}

__attribute__((noinline)) static void ay_mute()
{
    ay_write(AY_VOL_B, music_vol = 0);
}

void bg_music_mute()
{
    if (music_mode == SOUND_AY) ay_mute();
    music_vol = 0;
}

uint16_t sound_mode_next(uint16_t m)
{
    return ++m > SOUND_OFF ? SOUND_BEEPER : m;
}

// Ноты начинаются на границе кадра (длительности кратны четырём слотам), поэтому
// шаг - кадр. Нота затухает огибающей AY: форма 9 (спад до нуля) в начале ноты,
// период - R12.
void bg_music_frame()
{
    if (music_mode != SOUND_AY) return;

    uint16_t started = 0;
    uint8_t quiet = 1; // Все слоты кадра - пауза между нотами
    for (uint16_t i = music_chunks_per_frame; i; --i)
    {
        started |= bg_music_advance();
        quiet &= mus.silent;
    }

    music_vol = quiet ? 0 : 020;
    music_gap = mus.silent - quiet; // Звучавший кадр кончается паузой
    ay_out(mus.ay_period, music_vol);
    if (started) ay_write(AY_ENV_SHAPE, 011);
}

// Конец кадра: музыка пищалкой в свободном времени кадра; на AY - пауза между нотами
void bg_music_play()
{
        volatile uint16_t *t_count = (volatile uint16_t *)REG_TVE_COUNT;
        volatile union TVE_CSR *tve_csr = (volatile union TVE_CSR *)REG_TVE_CSR;
        const uint16_t limit = *(volatile uint16_t *)REG_TVE_LIMIT; // Длина кадра в отсчётах

        if (music_mode == SOUND_AY)
        {
            // Нота кончается с кадром: последняя четверть кадра - пауза, как слот-пауза
            // у пищалки. Без неё одинаковые ноты (галоп в бонусе) сливались в одну
            if (music_gap)
            {
                while (*t_count > (limit >> 2) && (tve_csr->reg & (1 << TVE_CSR_RUN)));
                ay_mute();
            }
            return;
        }
        if (music_mode != SOUND_BEEPER) return;

        // Новый кусочек не начинаем в последней восьмой части кадра (~11 мс)
        for (uint16_t i = music_chunks_per_frame; i; --i)
        {
            if (*t_count <= (limit >> 3) || !(tve_csr->reg & (1 << TVE_CSR_RUN))) break;
            bg_music_advance();
            if (!mus.silent) sound_pwm(mus.period, mus.chunk, 1);
        }
}
