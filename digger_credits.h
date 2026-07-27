#pragma once

#define RED   "\221"
#define GREEN "\222"
#define BLUE  "\223"
#define BLACK "\224"

#define CREDITS_ROW0   1  // Верхняя текстовая строка драйвера (подстройка на железе)
#define CREDITS_MARGIN 1  // Левый столбец поля (столбец 0 занят синей рамкой)
#define CREDITS_FIELD  15 // Ширина поля центрирования в знакоместах (столбцы 1..15)

// Текст кредитов об истории Digger, предварительно разбитый на строки <= 15  символов (разделитель '\n').
extern const char credits[];
