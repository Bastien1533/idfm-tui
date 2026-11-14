#ifndef TUI_LOADING_SCENE_H
#define TUI_LOADING_SCENE_H

#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stdlib.h>

#include "tui/scenes/scenes.h"

typedef struct loading_ctx
{
    struct ncprogbar *bar;
    struct ncplane *log_plane;
} loading_ctx;

scene_t *create_loading_scene(struct notcurses *nc,
                              struct ncplane *parent_plane, ui_ctx *parent_ctx);

#endif
