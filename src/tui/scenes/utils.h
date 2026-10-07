#ifndef SCENES_UTILS_H
#define SCENES_UTILS_H

#include <notcurses/notcurses.h>
#include "tui/ui/button.h"
#include "tui/ui/input.h"
#include "tui/scenes/scenes.h"

bool handle_click_buttons(ui_button **buttons, size_t n_buttons, int x, int y);
bool handle_click_inputs(scene_t *scene, ui_input **inputs, size_t n_inputs,
                         int x, int y,
                         void (*onclick)(scene_t *scene, ui_input *input));

#endif
