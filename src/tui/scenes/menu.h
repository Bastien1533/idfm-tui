#ifndef TUI_MENU_SCENE_H
#define TUI_MENU_SCENE_H

#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stdlib.h>

#include "tui/scenes/scenes.h"
#include "tui/ui/button.h"

scene_t *create_menu_scene(struct notcurses *nc, struct ncplane *parent_plane,
                           ui_ctx *parent_ctx);

#endif
