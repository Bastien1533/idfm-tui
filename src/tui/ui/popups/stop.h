#ifndef POPUPS_STOP_H
#define POPUPS_STOP_H

#include <notcurses/notcurses.h>

#include "algo/raptor.h"

typedef struct scene scene_t;
typedef struct ui_popup ui_popup;

typedef struct ui_input ui_input;
typedef struct ui_message ui_message;

ui_popup *create_stop_popup(scene_t *scene, struct stop *stop);

#endif
