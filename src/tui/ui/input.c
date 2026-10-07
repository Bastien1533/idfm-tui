#include <notcurses/notcurses.h>
#include "tui/ui/input.h"
#include "tui/utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

void move_input(ui_input *ipt, int x, int y)
{
        if (!ipt || ipt->box_plane == NULL)
                return;
        ncplane_move_yx(ipt->box_plane, x, y);
}

ui_input *create_input(struct ncplane *parent, ui_input_opts *input_opts)
{
        if (parent == NULL || input_opts == NULL || input_opts->rows < 3 ||
            input_opts->cols < 3) {
                errno = EINVAL;
                return NULL;
        }

        ui_input_opts *opts = malloc(sizeof(ui_input_opts));
        if (opts == NULL) {
                errno = ENOMEM;
                return NULL;
        }
        *opts = *input_opts;
        opts->id = opts->id ? strdup(opts->id) : NULL;

        struct ncplane_options box_opts = { .y = opts->y,
                                            .x = opts->x,
                                            .rows = opts->rows,
                                            .cols = opts->cols };
        struct ncplane *box_plane = ncplane_create(parent, &box_opts);

        uint64_t box_ch = 0;
        set_bg_color(box_plane, box_ch);

        ncplane_perimeter_rounded(box_plane, 0, box_ch, 0);

        // 2. Reader plane inside the box (y=1, x=1 so it stays inside the borders)
        struct ncplane_options input_plane_opts = {
                .y = 1, .x = 1, .rows = opts->rows - 2, .cols = opts->cols - 2
        };
        struct ncplane *input_plane =
                ncplane_create(box_plane, &input_plane_opts);

        uint64_t text_ch = 0;
        ncplane_set_channels(input_plane, text_ch);
        set_bg_color(input_plane, text_ch);

        struct ncreader_options ropts = {
                .flags = NCREADER_OPTION_CURSOR | NCREADER_OPTION_HORSCROLL,
                .tchannels = text_ch,
        };
        struct ncreader *reader = ncreader_create(input_plane, &ropts);
        if (reader == NULL) {
                free(opts->id);
                free(opts);
                ncplane_destroy(box_plane);
                ncplane_destroy(input_plane);
                return NULL;
        }
        ui_input *ipt = calloc(1, sizeof(ui_input));
        if (ipt == NULL) {
                free(opts->id);
                free(opts);
                ncplane_destroy(box_plane);
                ncplane_destroy(input_plane);
                ncreader_destroy(reader, NULL);
                errno = ENOMEM;
                return NULL;
        }
        ipt->opts = opts;
        ipt->box_plane = box_plane;
        ipt->reader = reader;
        return ipt;
}

ui_input *add_input(struct ncplane *parent, ui_input ***input_array,
                    size_t *input_n, ui_input_opts *opts)
{
        if (input_array == NULL || parent == NULL || opts == NULL)
                return NULL;

        ui_input *ipt = create_input(parent, opts);
        if (!ipt)
                return NULL;

        ui_input **new_array =
                realloc(*input_array, (*input_n + 1) * sizeof(ui_input *));

        if (!new_array) {
                free_input(ipt);
                return NULL;
        }

        *input_array = new_array;
        (*input_array)[*input_n] = ipt;
        (*input_n)++;
        return ipt;
}

void free_input(ui_input *ipt)
{
        if (ipt == NULL)
                return;
        if (ipt->opts != NULL) {
                free(ipt->opts->id);
                free(ipt->opts);
        }
        ncreader_destroy(ipt->reader, NULL);
        ncplane_destroy(ipt->box_plane);
        free(ipt);
}
