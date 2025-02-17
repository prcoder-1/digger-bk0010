#pragma once
#include <stdint.h>

//#define N 64 // Делитель периодов нот
#define N 20 // Делитель периодов нот
#define NV 4 // Делитель для звуковой процедуры "вибрато"
#define NM 16 // Делитель для фоновой музыки

// Периоды нот
#define	C3	(9122 / N)
#define	CS3	(8610 / N)
#define	D3	(8127 / N)
#define	DS3	(7671 / N)
#define	E3	(7240 / N)
#define	F3	(6834 / N)
#define	FS3	(6450 / N)
#define	G3	(6088 / N)
#define	GS3	(5747 / N)
#define	A3	(5424 / N)
#define	AS3	(5120 / N)
#define	B3	(4832 / N)

#define	C4	(4561 / N)
#define	CS4	(4305 / N)
#define	D4	(4063 / N)
#define	DS4	(3835 / N)
#define	E4	(3620 / N)
#define	F4	(3417 / N)
#define	FS4	(3225 / N)
#define	G4	(3044 / N)
#define	GS4	(2873 / N)
#define	A4	(2712 / N)
#define	AS4	(2560 / N)
#define	B4	(2416 / N)

#define	C5	(2280 / N)
#define	CS5	(2152 / N)
#define	D5	(2032 / N)
#define	DS5	(1918 / N)
#define	E5	(1810 / N)
#define	F5	(1708 / N)
#define	FS5	(1612 / N)
#define	G5	(1522 / N)
#define	GS5	(1437 / N)
#define	A5	(1356 / N)
#define	AS5	(1280 / N)
#define	B5	(1208 / N)

#define	C6	(1140 / N)
#define	CS6	(1076 / N)
#define	D6	(1016 / N)

#define REST 0xFF // Специальное значение - пауза (без звука)

#define T 64 // Множитель длительности

// Длительность нот
#define NS (T << 0)               //  1 * T
#define	NE (T << 1)               //  2 * T
#define	NQ (T << 2)               //  4 * T
#define	N6 ((T << 2) + (T << 1))  //  6 * T
#define	NH (T << 3)               //  8 * T
#define	N10 ((T << 3) + (T << 1)) // 10 * T
#define	N12 ((T << 3) + (T << 2)) // 12 * T
#define	NW (T << 4)               // 16 * T

// extern const uint8_t music0[][2]; // Музыка для режима "Бонус"
extern const uint8_t music_popcorn[145][2]; // Музыка "Popcorn"
// extern const uint8_t music2[][2]; // Траурный марш
// extern const uint8_t music_coin[][2]; // Звуки съедания монеток (камешков)

// void init_music(uint8_t x);
// void play_music(const uint8_t *music_ptr, uint8_t  **cur_ptr);
