#pragma once
#include <stdint.h>

#define N 96u // Делитель периодов нот

// Периоды нот
#define	C3	(9122u / N)
#define	CS3	(8610u / N)
#define	D3	(8127u / N)
#define	DS3	(7671u / N)
#define	E3	(7240u / N)
#define	F3	(6834u / N)
#define	FS3	(6450u / N)
#define	G3	(6088u / N)
#define	GS3	(5747u / N)
#define	A3	(5424u / N)
#define	AS3	(5120u / N)
#define	B3	(4832u / N)

#define	C4	(4561u / N)
#define	CS4	(4305u / N)
#define	D4	(4063u / N)
#define	DS4	(3835u / N)
#define	E4	(3620u / N)
#define	F4	(3417u / N)
#define	FS4	(3225u / N)
#define	G4	(3044u / N)
#define	GS4	(2873u / N)
#define	A4	(2712u / N)
#define	AS4	(2560u / N)
#define	B4	(2416u / N)

#define	C5	(2280u / N)
#define	CS5	(2152u / N)
#define	D5	(2032u / N)
#define	DS5	(1918u / N)
#define	E5	(1810u / N)
#define	F5	(1708u / N)
#define	FS5	(1612u / N)
#define	G5	(1522u / N)
#define	GS5	(1437u / N)
#define	A5	(1356u / N)
#define	AS5	(1280u / N)
#define	B5	(1208u / N)

#define	C6	(1140u / N)
#define	CS6	(1076u / N)
#define	D6	(1016u / N)

#define REST 0xFFu // Специальное значение - пауза (без звука)

#define T 15u // Множитель длительности

// Длительность нот
#define NS (T << 0)               //  1 * T
#define	NE (T << 1)               //  2 * T
#define	NQ (T << 2)               //  4 * T
#define	N6 ((T << 2) + (T << 1))  //  6 * T
#define	NH (T << 3)               //  8 * T
#define	N10 ((T << 3) + (T << 1)) // 10 * T
#define	N12 ((T << 3) + (T << 2)) // 12 * T
#define	NW (T << 4)               // 16 * T

extern const uint16_t music0[][2]; // Основная музыка "Popcorn"
// extern const uint16_t music1[][2]; // Музыка для режима "Бонус"
// extern const uint16_t music2[][2]; // Траурный марш
// extern const uint16_t music3[][2]; // tico-tico (сохранено для истории)

void play_music(const uint16_t *music_ptr, uint16_t  **cur_ptr);
