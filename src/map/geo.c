#include "map/geo.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "data/gtfs.h"
#include "map/map.h"

struct flat_pt latlon_to_flat(double lat, double lon, double lat_ref)
{
    double x = lon * cos(lat_ref * M_PI / 180.0);
    double y = lat;

    struct flat_pt pt = {
        .x = x,
        .y = y,
    };

    return pt;
}

struct flat_pt geo_to_flat(struct geo_pt geo_pt, double lat_ref)
{
    return latlon_to_flat(geo_pt.lat, geo_pt.lon, lat_ref);
}

struct cell_pos project_point(struct flat_pt point, struct map_view *map_view)
{
    struct cell_pos pos = {
        .col = (point.x - map_view->cx) * map_view->scale + map_view->cols / 2,
        .row = (map_view->cy - point.y) * map_view->scale * map_view->aspect
            + map_view->rows / 2
    };

    return pos;
}

struct flat_pt unproject_point(struct cell_pos point, struct map_view *map_view)
{
    struct flat_pt pt = { .x = (point.col - map_view->cols / 2.0)
                                  / map_view->scale
                              + map_view->cx,
                          .y = map_view->cy
                              - (point.row - map_view->rows / 2.0)
                                  / (map_view->scale * map_view->aspect) };

    return pt;
}

struct bbox map_view_bbox(struct map_view *view)
{
    if (view == NULL)
        return (struct bbox){ 0 };
    double halfw = (view->cols / 2.0) / view->scale;
    double halfh = (view->rows / 2.0) / (view->scale * view->aspect);

    struct bbox res = {
        .xmax = view->cx + halfw,
        .xmin = view->cx - halfw,
        .ymax = view->cy + halfh,
        .ymin = view->cy - halfh,
    };
    return res;
}

bool check_bbox_intersect(struct bbox box1, struct bbox box2)
{
    return (box1.xmin <= box2.xmax && box1.xmax >= box2.xmin)
        && (box1.ymin <= box2.ymax && box1.ymax >= box2.ymin);
}

struct bbox get_shape_bbox(struct flat_shape *shape)
{
    if (shape == NULL || shape->as.line.pts == NULL
        || shape->as.line.n_pts == 0)
        return (struct bbox){ 0 };

    struct flat_pt fp = shape->as.line.pts[0];
    double xmax = fp.x;
    double ymax = fp.y;
    double xmin = fp.x;
    double ymin = fp.y;
    for (size_t i = 0; i < shape->as.line.n_pts; i++)
    {
        struct flat_pt fp = shape->as.line.pts[i];

        xmax = xmax > fp.x ? xmax : fp.x;
        ymax = ymax > fp.y ? ymax : fp.y;
        xmin = xmin < fp.x ? xmin : fp.x;
        ymin = ymin < fp.y ? ymin : fp.y;
    }
    struct bbox res = {
        .xmax = xmax, .ymax = ymax, .xmin = xmin, .ymin = ymin
    };
    return res;
}

struct bbox get_bbox(struct flat_pt pt1, struct flat_pt pt2)
{
    struct bbox res = { .xmax = fmax(pt1.x, pt2.x),
                        .xmin = fmin(pt1.x, pt2.x),
                        .ymax = fmax(pt1.y, pt2.y),
                        .ymin = fmin(pt1.y, pt2.y) };
    return res;
}

bool compute_liang_barksy_line(struct bbox screen_box, struct flat_pt pt1,
                               struct flat_pt pt2, struct flat_pt *npt1,
                               struct flat_pt *npt2)
{
    double x1 = pt1.x;
    double x2 = pt2.x;
    double y1 = pt1.y;
    double y2 = pt2.y;

    double dx = x2 - x1;
    double dy = y2 - y1;

    double p[4] = { -dx, dx, -dy, dy };
    double q[4] = { x1 - screen_box.xmin, screen_box.xmax - x1,
                    y1 - screen_box.ymin, screen_box.ymax - y1 };
    double t_enter = 0.0;
    double t_exit = 1.0;

    for (size_t i = 0; i < 4; i++)
    {
        if (p[i] == 0)
        {
            if (q[i] < 0)
                return false;
        }
        else
        {
            double t = q[i] / p[i];
            if (p[i] < 0)
            {
                if (t > t_enter)
                    t_enter = t;
            }
            else
            {
                if (t < t_exit)
                    t_exit = t;
            }
        }
    }
    if (t_enter > t_exit)
        return false;

    struct flat_pt res_pt1 = {
        .x = x1 + t_enter * dx,
        .y = y1 + t_enter * dy,
    };
    struct flat_pt res_pt2 = {
        .x = x1 + t_exit * dx,
        .y = y1 + t_exit * dy,
    };

    *npt1 = res_pt1;
    *npt2 = res_pt2;
    return true;
}
