#include "digger_credits.h"

#define RED   "\221"
#define GREEN "\222"
#define BLUE  "\223"
#define BLACK "\224"

// Строки уже разбиты по <= 15 символов; разделитель '\n'. Печатается через
// драйвер дисплея ПЗУ (EMT) построчно с центрированием — см. print_credits().
const char credits[] =
    "The Canadian\n"
    "studio \n"
    RED "Windmill\n"
    "Software" GREEN "\n"
    "lead programmer\n"
    RED "Rob Stealth" GREEN " and\n"
    "his teammates\n"
    RED "Ray Ewan" GREEN " and\n"
    RED "Bill Montgomery" GREEN "\n"
    "released " RED "Digger" GREEN "\n"
    "in " BLUE "1983" GREEN " for\n"
    RED "IBM PC" GREEN ".\n"
    "In " BLUE "2026" RED " Digger" GREEN "\n"
    "was converted\n"
    "to run on the\n"
    "Soviet computer\n"
    RED "\xE2\xEB 0010-01" GREEN ".\n";
