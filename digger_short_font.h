#pragma once
#include <stdint.h>

// Сжатое представление спрайтов цифрового шрифта 12x12 пикселей.
// digit_rows - таблица 24 уникальных строк по 3 байт.
// digit_indices - для каждой из 10 цифр ('0'..'9') массив 12 индексов
// в digit_rows. Распаковка - в print_dec через временный буфер 36 байт.
extern const uint8_t digit_rows[24][3];
extern const uint8_t digit_indices[10][12];

extern const uint8_t game_over[12][27];
