#pragma once
#include <stdint.h>

// Кредиты (ZX0-сжатый текст, разбит на строки <=15 симв., исходный регистр,
// разделитель '\n', NUL в конце). Полноразмерный шрифт ПЗУ 8x10 (есть и
// строчные), строки центрируются по левой панели (см. blit_credits).
#define CREDITS_LINES 16
#define CREDITS_RAW_SIZE 214

extern const uint8_t credits_zx0[];
extern const uint16_t credits_zx0_size;
