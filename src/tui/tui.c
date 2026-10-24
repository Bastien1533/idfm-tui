#include "tui/protocol.h"
#include "tui/ui/selector.h"
#include <notcurses/notcurses.h>
#include <pthread.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

#include "tui/tui.h"
#include "tui/utils.h"
#include "tui/worker/worker.h"

#include "tui/scenes/scenes.h"
#include "tui/scenes/itinerary/itinerary.h"
#include "tui/scenes/menu.h"
#include "tui/scenes/loading.h"

int pipe_ui[2];
int pipe_logic[2];

int render(void)
{
        setlocale(LC_ALL, "");

        if (pipe(pipe_ui) == -1 || pipe(pipe_logic) == -1) {
                return EXIT_FAILURE;
        }
        int flags = fcntl(pipe_ui[0], F_GETFL, 0);
        fcntl(pipe_ui[0], F_SETFL,
              flags | O_NONBLOCK); // Ajouter le flag "Non Bloquant" au desc du pipe_ui;

        worker_agrs *wargs = calloc(1, sizeof(worker_agrs));
        if (wargs == NULL) {
                return EXIT_FAILURE;
        }
        wargs->pipe_logic_fd = pipe_logic[0];
        wargs->pipe_ui_fd = pipe_ui[1];
        pthread_t logic;
        pthread_create(&logic, NULL, logic_worker, wargs);

        struct notcurses_options opts = { .flags = NCOPTION_SUPPRESS_BANNERS };

        struct notcurses *nc = notcurses_init(&opts, NULL);
        if (!nc) {
                return EXIT_FAILURE;
        }
        notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);

        struct ncplane_options copts = {
                .y = 2, // Top-left Y offset relative to parent
                .x = 4, // Top-left X offset relative to parent
                .rows = 40, // Height (lines)
                .cols = 600, // Width (columns)
                .userptr = NULL, // Optional user data pointer
                .name = "my_plane" // Optional string label for debugging
        };

        struct ncplane *base_plane = notcurses_stdplane(nc);
        struct ncplane *child_plane = ncplane_create(base_plane, &copts);

        uint64_t ch = 0;
        set_bg_color(base_plane, ch);
        set_bg_color(child_plane, ch);
        notcurses_render(nc);

        // Tasks
        bool has_init_worker = false;
        bool has_init_requested = false;
        bool has_init_finished = false;

        ui_ctx *ctx = init_ctx(pipe_ui[0], pipe_logic[1]);

        scene_t *loading_scene = create_loading_scene(nc, base_plane, ctx);
        add_scene(loading_scene, ctx);

        scene_t *menu_scene = create_menu_scene(nc, base_plane, ctx);
        add_scene(menu_scene, ctx);

        struct ncinput ni;
        uint32_t key;

        size_t distant_counter = 0;
        while (1) {
                if (has_init_worker && !has_init_finished) {
                        switch_scene(ctx, menu_scene);
                        has_init_finished = true;
                } else if (ctx->curr_scene == NULL) {
                        switch_scene(ctx, loading_scene);
                }

                if (ctx->should_quit) {
                        logic_task_req *task_req =
                                malloc(sizeof(logic_task_req));
                        task_req->type = LOGIC_SHUTDOWN;
                        write(pipe_logic[1], &task_req,
                              sizeof(logic_task_req *));
                        quit_app(ctx, child_plane, nc);
                        return 0;
                }

                key = notcurses_get_nblock(nc, &ni);
                if (key == 'q')
                        ctx->should_quit = true;
                if (key != 0) {
                        if (ctx->curr_scene) {
                                ctx->curr_scene->handle_input(ctx->curr_scene,
                                                              &ni, key);
                        }
                }
                if (!has_init_worker && !has_init_requested) {
                        logic_task_req *task_req =
                                malloc(sizeof(logic_task_req));
                        task_req->type = LOGIC_TASK_INIT_REQ;
                        write(pipe_logic[1], &task_req,
                              sizeof(struct logic_task_req *));
                        has_init_requested = true;
                }

                ui_message *incoming_msg;
                if (read(pipe_ui[0], &incoming_msg,
                         sizeof(struct ui_message *)) ==
                    sizeof(struct ui_message *)) {
                        switch (incoming_msg->type) {
                        case UI_MSG_LOG:
                                if (ctx != NULL && ctx->curr_scene != NULL) {
                                        ncplane_printf(
                                                ctx->curr_scene->plane, "%s",
                                                incoming_msg->as.log.text);
                                }
                                free(incoming_msg->as.log.text);
                                free(incoming_msg);
                                break;
                        case UI_ERASE:
                                if (ctx->curr_scene) {
                                        switch_scene(ctx, NULL);
                                }
                                free(incoming_msg);
                                break;
                        case UI_INIT_FINISHED:
                                has_init_worker = true;
                                free(incoming_msg);
                                break;
                        case UI_STOP_RESULT_DATA:
                                if (ctx->curr_scene != NULL &&
                                    ctx->curr_scene->handle_message != NULL) {
                                        ctx->curr_scene->handle_message(
                                                ctx->curr_scene, incoming_msg);
                                } else {
                                        free_selector_items(
                                                &incoming_msg->as.res->items,
                                                &incoming_msg->as.res->n_res);
                                        free(incoming_msg->as.res);
                                        free(incoming_msg);
                                }
                                break;
                        case UI_SHAPE_RESULT_DATA:
                                if (ctx->curr_scene != NULL &&
                                    ctx->curr_scene->handle_message != NULL) {
                                        ctx->curr_scene->handle_message(
                                                ctx->curr_scene, incoming_msg);
                                } else {
                                        free(incoming_msg->as.shapes);
                                        free(incoming_msg);
                                }
                                break;
                        case UI_ITINERARY_RESULT_DATA:
                                if (ctx->curr_scene != NULL &&
                                    ctx->curr_scene->handle_message != NULL) {
                                        ctx->curr_scene->handle_message(
                                                ctx->curr_scene, incoming_msg);
                                } else {
                                        free_itinerary_result(
                                                incoming_msg->as
                                                        .itinerary_req_res);
                                        free(incoming_msg);
                                }
                                break;
                        default:
                                break;
                        }
                }

                notcurses_render(nc);
                usleep(16666);
        }

        notcurses_stop(nc);
        return EXIT_SUCCESS;
}
