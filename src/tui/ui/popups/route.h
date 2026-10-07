#ifndef POPUPS_ROUTE_H
#define POPUPS_ROUTE_H

#include "data/gtfs.h"
#include <notcurses/notcurses.h>

typedef struct scene scene_t;
typedef struct ui_popup ui_popup;

typedef struct ui_input ui_input;
typedef struct ui_message ui_message;

ui_popup *create_route_popup(scene_t *scene, struct route *route);

#endif
