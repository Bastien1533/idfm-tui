#ifndef TUI_MENU_SCENE_H
#define TUI_MENU_SCENE_H

#include <stdlib.h>
#include <stdbool.h>
#include <notcurses/notcurses.h>
#include "tui/ui/button.h"
#include "tui/scenes/scenes.h"

scene_t *create_menu_scene(struct notcurses *nc, struct ncplane *parent_plane,
                           ui_ctx *parent_ctx);

#endif
