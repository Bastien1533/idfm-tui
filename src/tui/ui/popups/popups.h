#ifndef POPUPS_H
#define POPUPS_H

#include <notcurses/notcurses.h>

typedef struct scene scene_t;
typedef struct ui_popup ui_popup;

typedef struct ui_input ui_input;
typedef struct ui_message ui_message;

typedef struct ui_popup
{
    char *title;
    char *id;
    void *payload;

    scene_t *parent_scene;
    struct ncplane *plane;

    void (*handle_input)(ui_popup *self, const ncinput *ni, uint32_t key);
    void (*handle_message)(ui_popup *self, ui_message *message);
} ui_popup;

void destroy_popup(ui_popup **popup);
struct ncplane *get_popup_plane(ui_popup *popup, ncplane_options *nc_opts);

#endif
