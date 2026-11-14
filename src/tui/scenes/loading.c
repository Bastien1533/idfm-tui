#include "tui/scenes/loading.h"

#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stdlib.h>

#include "tui/protocol.h"
#include "tui/scenes/scenes.h"
#include "tui/ui/button.h"
#include "tui/ui/map.h"
#include "tui/utils.h"

static void print_log(scene_t *scene, ui_message *message)
{
    loading_ctx *l_ctx = scene->payload;
    ncprogbar_set_progress(l_ctx->bar, message->as.progress.progress);
    ncplane_erase(l_ctx->log_plane);
    ncplane_printf_aligned(l_ctx->log_plane, 0, NCALIGN_CENTER, "%s",
                           message->as.progress.text);
    free(message->as.log.text);
    free(message);
}

static void receive_messages(scene_t *scene, ui_message *message)
{
    if (scene == NULL || message == NULL)
        return;
    if (message->type == UI_MSG_PROGRESS)
        print_log(scene, message);
}

void scene_loading_on_enter(scene_t *scene)
{
    if (scene == NULL || scene->ctx == NULL || scene->parent_ctx == NULL
        || scene->parent == NULL)
    {
        return;
    }

    unsigned rows, cols;
    ncplane_dim_yx(scene->parent, &rows, &cols);
    struct ncplane_options nopts = { .y = 0,
                                     .x = 0,
                                     .rows = rows,
                                     .cols = cols,
                                     .name = "scene_loading_plane" };
    on_scene_enter(scene, &nopts);

    const char *title_art[] = { " ___ ____  _____ __  __   _____ _   _ ___ ",
                                "|_ _|  _ \\|  ___|  \\/  | |_   _| | | |_ _|",
                                " | || | | | |_  | |\\/| |   | | | | | || | ",
                                " | || |_| |  _| | |  | |   | | | |_| || | ",
                                "|___|____/|_|   |_|  |_|   |_|  \\___/|___|" };

    // Allocate a small sub-plane for the header title
    struct ncplane_options opts = {
        .y = 2,
        .x = NCALIGN_CENTER, // Center align in parent
        .rows = 30,
        .cols = 52,
        .flags = NCPLANE_OPTION_HORALIGNED
    };

    struct ncplane *title_plane = ncplane_create(scene->plane, &opts);

    // Apply bright yellow text styling
    uint64_t t_channels = 0;

    set_bg_color(title_plane, t_channels);

    // Print lines
    for (int i = 0; i < 5; i++)
    {
        ncplane_printf_aligned(title_plane, i, NCALIGN_CENTER, "%s",
                               title_art[i]);
    }

    uint32_t bar_ch = 0;
    ncchannel_set(&bar_ch, 0x4bc0ad);
    ncprogbar_options prog_opts = {
        .ulchannel = bar_ch,
        .urchannel = bar_ch,
        .blchannel = bar_ch,
        .brchannel = bar_ch,
        .flags = 0,
    };

    struct ncplane_options plane_opts = {
        .y = rows / 2 - 1,
        .x = cols / 2 - (40 / 2),
        .rows = 2,
        .cols = 40,
        .userptr = NULL,
        .name = "bar_plane",
        .flags = 0,
    };
    struct ncplane *bar_plane = ncplane_create(scene->plane, &plane_opts);
    struct ncprogbar *bar = ncprogbar_create(bar_plane, &prog_opts);

    struct ncplane_options log_plane_opts = {
        .y = rows / 2 + 1,
        .x = cols / 2 - (50 / 2),
        .rows = 2,
        .cols = 50,
        .userptr = NULL,
        .name = "log_plane",
        .flags = 0,
    };

    struct ncplane *log_plane = ncplane_create(scene->plane, &log_plane_opts);
    ncplane_set_scrolling(log_plane, true);

    uint64_t channels = 0;
    set_bg_color(log_plane, channels);

    loading_ctx *l_ctx = scene->payload;
    l_ctx->bar = bar;
    l_ctx->log_plane = log_plane;

    return;
}

scene_t *create_loading_scene(struct notcurses *nc,
                              struct ncplane *parent_plane, ui_ctx *parent_ctx)
{
    if (nc == NULL || parent_plane == NULL)
        return NULL;

    ui_scene_ctx *ctx = init_scene_ctx(NULL);
    if (ctx == NULL)
        return NULL;

    scene_t *scene = calloc(1, sizeof(scene_t));
    if (scene == NULL)
    {
        free_hashmap(ctx->button_map);
        free(ctx);
        return NULL;
    }

    loading_ctx *loading_payload = calloc(1, sizeof(loading_ctx));
    if (loading_payload == NULL)
    {
        destroy_scene(scene);
        return NULL;
    }

    scene->type = UI_LOADING_SCENE;
    scene->ctx = ctx;
    scene->parent_ctx = parent_ctx;
    scene->parent = parent_plane;
    scene->payload = loading_payload;
    scene->handle_input = handle_scene_input;
    scene->handle_message = receive_messages;
    scene->on_enter = scene_loading_on_enter;
    scene->on_exit = on_scene_exit;
    scene->id = strdup("loading_scene");
    scene->nc = nc;

    return scene;
}
