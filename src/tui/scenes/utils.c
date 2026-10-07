#include "tui/ui/button.h"
#include "tui/ui/input.h"
#include "tui/ui/utils.h"
#include "tui/scenes/scenes.h"

#include <stdbool.h>
#include <stddef.h>

bool handle_click_buttons(ui_button **buttons, size_t n_buttons, int x, int y)
{
        if (buttons == NULL)
                return false;

        for (size_t i = 0; i < n_buttons; i++) {
                ui_button *btn = buttons[i];
                if (!btn || btn->opts == NULL || btn->opts->onclick == NULL)
                        continue;

                if (point_in_btn(x, y, btn)) {
                        void (*callback)(void *) = btn->opts->onclick;
                        void *payload = btn->opts->onclick_payload;
                        callback(payload);

                        return true;
                }
        }
        return false;
}

bool handle_click_inputs(scene_t *scene, ui_input **inputs, size_t n_inputs,
                         int x, int y,
                         void (*onclick)(scene_t *scene, ui_input *input))
{
        if (inputs == NULL)
                return false;

        for (size_t i = 0; i < n_inputs; i++) {
                ui_input *input = inputs[i];
                if (!input)
                        continue;

                if (point_in_input(x, y, input)) {
                        onclick(scene, input);
                        return true;
                }
        }
        return false;
}
