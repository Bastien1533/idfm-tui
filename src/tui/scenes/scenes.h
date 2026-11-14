#ifndef TUI_SCENES_H
#define TUI_SCENES_H

#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stdlib.h>

#include "tui/protocol.h"
#include "tui/ui/popups/popups.h"

typedef struct ui_button ui_button;
typedef struct ui_selector ui_selector;
typedef struct ui_input ui_input;
typedef struct ui_popup ui_popup;
typedef struct hash_map hash_map;
typedef struct scene scene_t;

typedef enum
{
    UI_MENU_SCENE,
    UI_ITINERARY_SCENE,
    UI_LOADING_SCENE,
    UI_MAP_SCENE,
} ui_scene_type;

typedef struct
{
    size_t n_buttons;
    struct hash_map *button_map;
    ui_button **buttons;

    ui_input *focused_reader;
    ui_input **inputs;
    size_t n_inputs;

    size_t n_selectors;
    ui_selector **selectors;

    ui_popup *popup;
} ui_scene_ctx;

typedef struct ui_ipc
{
    int pipe_ui_r;
    int pipe_logic_w;
    int request_seq;
} ui_ipc;

typedef struct ui_ctx
{
    size_t n_scenes;
    struct scene **scenes;
    struct hash_map *scenes_map;

    scene_t *curr_scene;

    // Flags
    bool should_quit;
    scene_t *next_scene;

    ui_ipc ipc;
} ui_ctx;

struct scene
{
    char *id;
    ui_scene_type type;
    ui_scene_ctx *ctx;
    void *payload;

    struct ncplane *plane;
    struct ncplane *parent;

    ui_ctx *parent_ctx;
    struct notcurses *nc;

    void (*on_enter)(scene_t *self);
    void (*handle_input)(scene_t *self, const ncinput *ni, uint32_t key);
    void (*handle_message)(scene_t *self, ui_message *message);
    void (*render)(scene_t *self);
    void (*on_exit)(scene_t *self);
};

ui_ctx *init_ctx(int pipe_ui_r, int pipe_logic_w);
ui_scene_ctx *init_scene_ctx(ui_scene_ctx *ctx);

void on_scene_enter(scene_t *scene, ncplane_options *nopts);
void on_scene_exit(scene_t *scene);
void handle_scene_input(scene_t *self, const ncinput *ni, uint32_t key);

bool add_scene(scene_t *scene, ui_ctx *ctx);
bool switch_scene(ui_ctx *ctx, scene_t *new);
void destroy_scene(struct scene *scene);

#endif
