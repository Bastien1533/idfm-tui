#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "data/gtfs.h"
#include "map/geo.h"
#include "map/map.h"
#include "map/shapes.h"

#define SX_M 111320.0
#define SY_M 111132.0

double seg_len(struct flat_pt a, struct flat_pt b)
{
    double dx =
        (b.x - a.x) * SX_M; /* flat degrees → metres (cos already in x) */
    double dy = (b.y - a.y) * SY_M; /* lat degrees  → metres */
    return sqrt(dx * dx + dy * dy);
}
// t is the position of the seg_pt on the ab seg (0->1)
double project(struct flat_pt a, struct flat_pt b, struct flat_pt pt,
               struct flat_pt *seg_pt, double *t)
{
    struct flat_pt d = { .x = b.x - a.x, .y = b.y - a.y }; // AB seg vector
    double d_d = d.x * d.x + d.y * d.y;
    if (d_d == 0)
    {
        *t = 0;
        *seg_pt = a;
        return seg_len(a, pt);
    }

    struct flat_pt apt = { .x = pt.x - a.x, .y = pt.y - a.y }; // AP seg vector
    double apt_d = d.x * apt.x + d.y * apt.y;

    *t = apt_d / d_d;
    if (*t < 0.0)
        *t = 0.0;
    if (*t > 1.0)
        *t = 1.0;

    struct flat_pt new_pt = { .x = a.x + *t * d.x, .y = a.y + *t * d.y };
    *seg_pt = new_pt;

    return seg_len(new_pt, pt);
    ;
}

void populate_cuml_len(struct flat_shape *shape, double **cum)
{
    if (shape->type != LINE)
        return;
    (*cum) = calloc(shape->as.line.n_pts, sizeof(double));
    for (size_t i = 1; i < shape->as.line.n_pts; i++)
    {
        struct flat_pt a = shape->as.line.pts[i - 1];
        struct flat_pt b = shape->as.line.pts[i];
        (*cum)[i] = (*cum)[i - 1] + seg_len(a, b);
    }
}

void get_along_distances(struct flat_shape shp, struct flat_pt dep_fp,
                         struct flat_pt arr_fp, double *cum,
                         double *low_along_dist, double *high_along_dist)
{
    int min_dist_seg_idx_dep = -1;
    double min_dist_dep = INT_MAX;
    double min_dep_t = 0;

    int min_dist_seg_idx_arr = -1;
    double min_dist_arr = INT_MAX;
    double min_arr_t = 0;

    for (size_t i = 1; i < shp.as.line.n_pts; i++)
    {
        double t;
        struct flat_pt new_pt;
        // dep
        double dist = project(shp.as.line.pts[i - 1], shp.as.line.pts[i],
                              dep_fp, &new_pt, &t);
        if (dist < min_dist_dep && dist < 150)
        {
            min_dist_dep = dist;
            min_dist_seg_idx_dep = i - 1;
            min_dep_t = t;
        }
        // arr
        dist = project(shp.as.line.pts[i - 1], shp.as.line.pts[i], arr_fp,
                       &new_pt, &t);
        if (dist < min_dist_arr && dist < 150)
        {
            min_dist_arr = dist;
            min_dist_seg_idx_arr = i - 1;
            min_arr_t = t;
        }
    }

    if (min_dist_seg_idx_arr == -1 || min_dist_seg_idx_dep == -1)
    {
        *low_along_dist = -1;
        *high_along_dist = -1;
        return;
    }

    double along_dep = cum[min_dist_seg_idx_dep]
        + min_dep_t
            * seg_len(shp.as.line.pts[min_dist_seg_idx_dep],
                      shp.as.line.pts[min_dist_seg_idx_dep + 1]);
    double along_arr = cum[min_dist_seg_idx_arr]
        + min_arr_t
            * seg_len(shp.as.line.pts[min_dist_seg_idx_arr],
                      shp.as.line.pts[min_dist_seg_idx_arr + 1]);

    *low_along_dist = fmin(along_dep, along_arr);
    *high_along_dist = fmax(along_dep, along_arr);
}

bool cut_trip_flatshape(struct flat_shape shp, struct stop *dep,
                        struct stop *arr, double lat_ref,
                        struct flat_shape *new_shp)
{
    if (shp.type != LINE)
        return false;
    double *cum;
    populate_cuml_len(&shp, &cum);

    struct flat_pt dep_fp =
        latlon_to_flat(dep->stop_lat, dep->stop_lon, lat_ref);
    struct flat_pt arr_fp =
        latlon_to_flat(arr->stop_lat, arr->stop_lon, lat_ref);

    double low_along_dist;
    double high_along_dist;

    get_along_distances(shp, dep_fp, arr_fp, cum, &low_along_dist,
                        &high_along_dist);
    if (low_along_dist == -1 || high_along_dist == -1)
    {
        free(cum);
        return false;
    }

    struct flat_pt *new_pts = calloc(shp.as.line.n_pts, sizeof(struct flat_pt));
    size_t new_pts_n = 0;
    for (size_t i = 1; i < shp.as.line.n_pts; i++)
    {
        size_t seg_idx = i - 1;
        if (cum[seg_idx + 1] <= low_along_dist)
            continue;
        if (cum[seg_idx] >= high_along_dist)
            continue;

        struct flat_pt a = shp.as.line.pts[i - 1];
        struct flat_pt b = shp.as.line.pts[i];
        if (cum[seg_idx] >= low_along_dist
            && cum[seg_idx + 1] <= high_along_dist) // Fully inside
        {
            new_pts[new_pts_n++] = a;
            new_pts[new_pts_n] = b;
        }
        else if (cum[seg_idx] < low_along_dist
                 && cum[seg_idx + 1] > low_along_dist)
        {
            struct flat_pt new_pt;
            double t;
            double dist = project(a, b, dep_fp, &new_pt, &t);
            new_pts[new_pts_n++] = new_pt;
            new_pts[new_pts_n] = b;
        }
        else if (cum[seg_idx] < high_along_dist
                 && cum[seg_idx + 1] > high_along_dist)
        {
            struct flat_pt new_pt;
            double t;
            double dist = project(a, b, arr_fp, &new_pt, &t);
            new_pts[new_pts_n++] = a;
            new_pts[new_pts_n] = new_pt;
        }
    }
    new_pts_n++;
    new_pts = realloc(new_pts, new_pts_n * sizeof(struct flat_pt));
    free(cum);
    free(shp.as.line.pts);

    *new_shp = shp;
    new_shp->as.line.n_pts = new_pts_n;
    new_shp->as.line.pts = new_pts;
    new_shp->bbox = get_shape_bbox(new_shp);
    return new_shp;
}