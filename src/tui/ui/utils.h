#ifndef UI_UTILS_H
#define UI_UTILS_H

#define RATP_DEEP_BLUE 0x050d9e
#define RATP_BEIGE_WHITE 0xFCF4DC

#include <notcurses/notcurses.h>
#include "tui/ui/button.h"
#include "tui/ui/input.h"

bool point_in_btn(int x, int y, ui_button *btn);
bool point_in_input(int x, int y, ui_input *input);

#endif
