#include "map/map.h"

#include <assert.h>
#include <math.h>
#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map/geo.h"
#include "map/shapes.h"
#include "map/utils.h"
#include "tui/worker/worker.h"

void map_print_frame(struct map_view *view, struct map_cell *cells,
                     size_t n_cells)
{
    for (size_t y = 0; y < view->rows; y++)
    {
        for (size_t x = 0; x < view->cols; x++)
        {
            struct map_cell *cell = cells + (y * view->cols) + x;
            if (cell->ch != 0)
                putc(cell->ch, stdout);
            else
                putc(' ', stdout);
        }
        putc('\n', stdout);
    }
}

void map_print_frame_nc(struct map_view *view, struct map_cell *cells,
                        size_t n_cells, struct ncplane *plane)
{
    ncplane_erase(plane);
    for (size_t y = 0; y < view->rows; y++)
    {
        for (size_t x = 0; x < view->cols; x++)
        {
            struct map_cell *cell = cells + (y * view->cols) + x;
            struct nccell c = { 0 };

            if (cell->ch != 0
                && (view->scale < cell->max_scale || cell->max_scale == -1)
                && (view->scale > cell->min_scale || cell->min_scale == -1))
            {
                nccell_load_egc32(plane, &c, cell->ch);
            }
            else
            {
                nccell_load_egc32(plane, &c, ' ');
            }
            nccell_set_fg_rgb(&c, cell->fg);
            nccell_set_bg_rgb(&c, cell->bg);
            ncplane_putc_yx(plane, y, x, &c);
        }
    }
}

void map_fit(struct map_view *view, struct bbox bbox)
{
    double width = bbox.xmax - bbox.xmin;
    double height = bbox.ymax - bbox.ymin;

    view->cx = (bbox.xmax + bbox.xmin) / 2;
    view->cy = (bbox.ymax + bbox.ymin) / 2;

    size_t pad = 10;
    size_t u_cols = view->cols - pad;
    size_t u_rows = view->rows - pad;

    double sx = u_cols / width;
    double sy = u_rows / (height * view->aspect);
    double scale = sx < sy ? sx : sy;

    view->u_cols = u_cols;
    view->u_rows = u_rows;
    view->scale = scale;

    view->view_box = map_view_bbox(view);
}

void map_fit_shapes(struct map_view *view, struct flat_shape *shapes,
                    size_t n_shapes)
{
    if (view == NULL || shapes == NULL || n_shapes == 0)
        return;

    struct flat_shape fs = shapes[0];

    double xmax = -INFINITY, ymax = -INFINITY;
    double xmin = INFINITY, ymin = INFINITY;
    size_t used = 0;

    for (size_t i = 0; i < n_shapes; i++)
    {
        if (!shape_valid(&shapes[i]))
            continue;
        xmax = fmax(xmax, shapes[i].bbox.xmax);
        ymax = fmax(ymax, shapes[i].bbox.ymax);
        xmin = fmin(xmin, shapes[i].bbox.xmin);
        ymin = fmin(ymin, shapes[i].bbox.ymin);
        used++;
    }
    if (used == 0 || xmax < xmin || ymax < ymin)
        return;

    struct bbox bbox = {
        .xmax = xmax,
        .xmin = xmin,
        .ymax = ymax,
        .ymin = ymin,
    };
    map_fit(view, bbox);
}

struct map_view map_init(size_t rows, size_t cols, double lat_ref,
                         double origin_x, double origin_y)
{
    struct map_view view = {
        .aspect = 0.5,
        .cols = cols,
        .rows = rows,
        .cx = cols / 2,
        .cy = rows / 2,
        .u_cols = cols,
        .u_rows = rows,
        .scale = 1,
        .origin_x = origin_x,
        .origin_y = origin_y,
    };
    return view;
}

void map_move_view(struct map_view *view, int off_x, int off_y)
{
    double dx = off_x / view->scale; // cells -> world (x)
    double dy = off_y / (view->scale * view->aspect); // cells -> world (y)
    double nx = view->cx + dx;
    double ny = view->cy - dy;

    (*view).cx = nx;
    (*view).cy = ny;

    (*view).view_box = map_view_bbox(view);
}

void map_zoom(struct map_view *view, double scale, int mouse_x, int mouse_y)
{
    if (view->scale * scale <= 0.5 && scale < 1)
        return;
    if (mouse_x < 0 || mouse_y < 0)
    { /* keyboard: zoom about center */
        view->scale *= scale;
    }
    else
    {
        struct cell_pos mouse = { .row = mouse_y, .col = mouse_x };
        struct flat_pt w = unproject_point(mouse, view); /* old scale */

        view->scale *= scale;
        /* re-center so w stays under the cursor */
        view->cx = w.x - (mouse_x - view->cols / 2.0) / view->scale;
        view->cy =
            w.y + (mouse_y - view->rows / 2.0) / (view->scale * view->aspect);
    }
    view->view_box = map_view_bbox(view);
}

void handle_map_input(const ncinput *ni, uint32_t key, struct map_view *view)
{
    if (ni == NULL)
        return;

    bool is_press =
        (ni->evtype == NCTYPE_PRESS || ni->evtype == NCTYPE_UNKNOWN);
    bool is_release = (ni->evtype == NCTYPE_RELEASE);
    bool is_repeat = (ni->evtype == NCTYPE_REPEAT);

    if (key == NCKEY_UP)
    {
        map_move_view(view, 0, -5);
        return;
    }
    if (key == NCKEY_DOWN)
    {
        map_move_view(view, 0, 5);
        return;
    }
    if (key == NCKEY_LEFT)
    {
        map_move_view(view, -5, 0);
        return;
    }
    if (key == NCKEY_RIGHT)
    {
        map_move_view(view, 5, 0);
        return;
    }
    if (key == '=' || key == NCKEY_SCROLL_DOWN)
    {
        map_zoom(view, 1.1, ni->x - view->origin_x, ni->y - view->origin_y);
        return;
    }
    if (key == '-' || key == NCKEY_SCROLL_UP)
    {
        map_zoom(view, 0.9, ni->x - view->origin_x, ni->y - view->origin_y);
        return;
    }
}

void draw_map(size_t n_shapes, struct flat_shape *flat_shapes,
              struct map_view view, struct map_cell **cells,
              struct ncplane *base_plane)
{
    size_t n_painted = 0;
    for (size_t i = 0; i < n_shapes; i++)
    {
        struct flat_shape fshape = flat_shapes[i];
        paint_flat_shape(fshape, &view, *cells, &n_painted);
    }
    map_print_frame_nc(&view, *cells, view.rows * view.cols, base_plane);
}

void map_render_loop(void)
{
    worker_data *wdata = calloc(1, sizeof(worker_data));
    if (wdata == NULL)
        return;
    init_worker(1, wdata);
    size_t n_shapes = wdata->n_shapes;

    struct notcurses_options opts = { .flags = NCOPTION_SUPPRESS_BANNERS };

    unsigned rows;
    unsigned cols;
    struct notcurses *nc = notcurses_init(&opts, NULL);
    notcurses_term_dim_yx(nc, &rows, &cols);
    notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);

    struct map_view view = map_init(rows, cols, wdata->lat_ref, 0, 0);
    struct flat_pt a =
        geo_to_flat((struct geo_pt){ .lon = 13.3951309, .lat = 51.5173885 },
                    wdata->lat_ref);
    struct flat_pt b =
        geo_to_flat((struct geo_pt){ .lon = 12.3951309, .lat = 52.5173885 },
                    wdata->lat_ref);
    struct bbox bbox = { .xmin = fmin(a.x, b.x),
                         .xmax = fmax(a.x, b.x),
                         .ymin = fmin(a.y, b.y),
                         .ymax = fmax(a.y, b.y) };

    map_fit(&view, bbox);

    size_t n_flat_shapes = 0;
    struct flat_shape *flat_shapes = flat_shapes_from_trips(
        wdata->trips, wdata->n_trips, wdata->shape_map, wdata->n_shapes,
        &n_flat_shapes, wdata->n_stops, wdata->lat_ref);
    map_fit_shapes(&view, flat_shapes, n_flat_shapes);

    struct ncplane *base_plane = notcurses_stdplane(nc);
    struct map_cell *cells;
    while (1)
    {
        cells = calloc(view.rows * view.cols, sizeof(struct map_cell));
        struct ncinput ni;
        uint32_t key = notcurses_get_blocking(nc, &ni);
        handle_map_input(&ni, key, &view);
        draw_map(n_flat_shapes, flat_shapes, view, &cells, base_plane);
        char *coords = NULL;

        struct flat_pt view_geo = { .x = view.cx, .y = view.cy };
        struct cell_pos view_pos = project_point(view_geo, &view);
        asprintf(&coords, "coords %i %i | %i %i | zoom %f \n", ni.x, ni.y,
                 view_pos.col, view_pos.row, view.scale / 200);
        ncplane_putstr_yx(base_plane, 10, 10, coords);

        free(coords);
        notcurses_render(nc);
        fflush(stdout);
        free(cells);
    }

    // block until a key, so the alt-screen stays up:

    notcurses_stop(nc);

    free(cells);
}
