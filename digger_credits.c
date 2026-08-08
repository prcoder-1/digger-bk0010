#include "digger_credits.h"

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
    "released \n"
    RED "Digger" GREEN " in " BLUE "1983" GREEN "\n"
    "for " RED "IBM PC" GREEN ".\n"
    "\n"
    "In " BLUE "2026" RED " Digger" GREEN "\n"
    "was converted\n"
    "to run on the\n"
    "Soviet computer\n"
    RED "\xE2\xEB 0010-01" GREEN ".\n";

// Названия клавиш БК заданы байтами KOI-8: "\xF3\xE2\xF2" = СБР, "\xF3\xF4\xEF\xF0" = СТОП
const char keys_help[] =
    BLUE "Controls:\n" GREEN "\n"
    "\n"
    RED "ARROWS" GREEN " or\n"
    RED "Joystick" GREEN " -\n"
    "Move Digger\n"
    "\n"
    RED "SPACE" GREEN " or\n"
    RED "Joy Button" GREEN " -\n"
    "Fireball\n"
    "\n"
    RED "   S" GREEN " - Sound FX\n"
    RED "   M" GREEN " - Music   \n"
#if defined(DEBUG)
    RED "   D" GREEN " - Difficul\n"
    RED "   L" GREEN " - Life    \n"
    RED "   N" GREEN " - Next Lev\n"
#endif
    RED " \xF3\xE2\xF2" GREEN " - Pause   \n"
    RED "\xF3\xF4\xEF\xF0" GREEN " - Restart \n";
