#ifndef TUI_MAP_SCENE_H
#define TUI_MAP_SCENE_H

#include <stdlib.h>
#include <stdbool.h>
#include <notcurses/notcurses.h>

#include "tui/ui/map.h"

typedef struct ui_ctx ui_ctx;

typedef struct map_ctx {
        ui_map *map;
        size_t pending_seq;
} map_ctx;

scene_t *create_map_scene(struct notcurses *nc, struct ncplane *parent_plane,
                          ui_ctx *parent_ctx);

#endif
