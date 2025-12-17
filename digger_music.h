#pragma once
#include <stdint.h>

#define N 40u // Делитель периодов нот
#define NV 4u // Дополнительный делитель для звуковой процедуры "вибрато"

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

// Длительность нот
#define NS 1
#define	NE 2
#define	NQ 4
#define	N6 6
#define	NH 8
#define	N10 10
#define	N12 12
#define	NW 16

// Основная музыка "Popcorn"
extern const uint16_t popcorn_periods[];
extern const uint8_t popcorn_durations[];

// Музыка для режима "Бонус"
extern const uint16_t bonus_periods[];
extern const uint8_t popcorn_durations[];
