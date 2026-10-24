#include "data/gtfs.h"
#include "map/style.h"
#include "utils/hashmap.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "map/map.h"
#include "map/geo.h"
#include "map/shapes.h"
#include <math.h>
#include <limits.h>
#include <string.h>

struct flat_shape gtfs_shape_to_flat(gfts_shp *shape, double lat_ref,
                                     struct flat_pt **out, size_t *n_out,
                                     struct shape_style style)
{
        if (shape == NULL || shape->points_n == 0)
                return (struct flat_shape){ 0 };

        struct flat_pt fp = latlon_to_flat(shape->points[0]->shape_pt_lat,
                                           shape->points[0]->shape_pt_lon,
                                           lat_ref);

        double xmax = fp.x;
        double ymax = fp.y;
        double xmin = fp.x;
        double ymin = fp.y;

        struct flat_pt *out_t = calloc(shape->points_n, sizeof(struct flat_pt));
        if (out_t == NULL)
                return (struct flat_shape){ 0 };

        *out = out_t;
        for (size_t i = 0; i < shape->points_n; i++) {
                struct flat_pt fp =
                        latlon_to_flat(shape->points[i]->shape_pt_lat,
                                       shape->points[i]->shape_pt_lon, lat_ref);

                xmax = xmax > fp.x ? xmax : fp.x;
                ymax = ymax > fp.y ? ymax : fp.y;
                xmin = xmin < fp.x ? xmin : fp.x;
                ymin = ymin < fp.y ? ymin : fp.y;

                (*out)[i] = fp;
                (*n_out)++;
        }
        struct bbox bbox = {
                .xmax = xmax,
                .ymax = ymax,
                .xmin = xmin,
                .ymin = ymin,
        };
        struct flat_shape flat_shape = {
                .bbox = bbox,
                .as.line = {
                    .pts = *out,
                    .n_pts = *n_out,
                },
                .min_scale = style.min_scale,
                .max_scale = style.max_scale,
                .color = style.color
        };
        return flat_shape;
}

struct flat_shape *gtfs_pathways_to_flat(struct pathway **pathways,
                                         size_t n_pathways, double lat_ref,
                                         size_t *n_shapes)
{
        if (pathways == NULL || n_pathways == 0)
                return NULL;

        double xmax = INT_MIN;
        double ymax = INT_MIN;
        double xmin = INT_MAX;
        double ymin = INT_MAX;

        struct flat_shape *out_shapes =
                calloc(n_pathways, sizeof(struct pathway));
        if (out_shapes == NULL)
                return NULL;

        for (size_t i = 0; i < n_pathways; i++) {
                struct pathway *path = pathways[i];
                if (path == NULL || path->from_stop == NULL ||
                    path->to_stop == NULL)
                        continue;
                struct flat_pt fp_start =
                        latlon_to_flat(path->from_stop->stop_lat,
                                       path->from_stop->stop_lon, lat_ref);

                struct flat_pt fp_end = latlon_to_flat(path->to_stop->stop_lat,
                                                       path->to_stop->stop_lon,
                                                       lat_ref);

                xmax = fmax(fmax(xmax, fp_start.x), fp_end.x);
                ymax = fmax(fmax(ymax, fp_start.y), fp_end.y);
                xmin = fmin(fmin(xmin, fp_start.x), fp_end.x);
                ymin = fmin(fmin(ymin, fp_start.y), fp_end.y);

                struct flat_pt *points = calloc(2, sizeof(struct flat_pt));
                if (points == NULL)
                        continue;
                points[0] = fp_start;
                points[1] = fp_end;

                struct flat_shape shape = {
                    .bbox = {
                        .xmax = xmax,
                        .xmin = xmin,
                        .ymax = ymax,
                        .ymin = ymin,
                    },
                    .color = 0x878787,
                    .as.line = {
                        .pts = points,
                        .n_pts = 2,
                    },
                    .min_scale = 300,
                    .max_scale = -1,
                };

                out_shapes[i] = shape;
                (*n_shapes)++;
        }
        return out_shapes;
}

static struct flat_shape stop_to_label(struct stop *stop, double lat_ref,
                                       struct shape_style style)
{
        struct flat_pt fp =
                latlon_to_flat(stop->stop_lat, stop->stop_lon, lat_ref);

        double mx = fp.x;
        double my = fp.y;
        struct flat_shape shape = {
                .type = TEXT,
                .bbox = { .xmax = mx, .xmin = mx, .ymax = my, .ymin = my },
                .color = style.color,
                .as.text.text = strdup(stop->stop_name),
                .as.text.mx = mx,
                .as.text.my = my,
                .min_scale = style.min_scale,
                .max_scale = style.max_scale,
        };
        return shape;
}

static struct flat_shape stop_to_circle(struct stop *stop, double lat_ref,
                                        struct shape_style style)
{
        struct flat_pt fp =
                latlon_to_flat(stop->stop_lat, stop->stop_lon, lat_ref);

        double mx = fp.x;
        double my = fp.y;

        struct flat_shape shape = {
        .type = CIRCLE,
        .bbox = { .xmax = mx, .xmin = mx, .ymax = my, .ymin = my },
        .color = style.color,
        .as.circle = {
            .radius = style.radius,
            .mx = mx,
            .my = my,
        },
        .min_scale = style.min_scale,
        .max_scale = style.max_scale,
    };
        return shape;
}

static bool trip_to_line_shape(struct trip *trip, struct hash_map *shape_map,
                               double lat_ref, struct flat_shape *dst)
{
        struct shape *shp = hash_map_get(shape_map, trip->shape_id);
        if (!shp || !(trip->route))
                return false;

        struct shape_style style = style_route(trip->route);

        struct flat_pt *out = NULL;
        size_t n_out = 0; /* gtfs_shape_to_flat increments it */
        struct flat_shape fshp =
                gtfs_shape_to_flat(shp, lat_ref, &out, &n_out, style);
        if (out == NULL || n_out < 2) {
                free(out);
                return false;
        }
        *dst = fshp;
        return true;
}

static size_t stop_to_flat_shape(struct stop *s, struct flat_shape *flat_shapes,
                                 size_t n_flat_shapes, double lat_ref)
{
        size_t cur_n_flat_shapes = n_flat_shapes;
        struct shape_style style = style_stop(s);
        struct flat_shape circle_shp = stop_to_circle(s, lat_ref, style);
        flat_shapes[cur_n_flat_shapes++] = circle_shp;
        if (style.label) {
                struct flat_shape label_shp = stop_to_label(s, lat_ref, style);
                flat_shapes[cur_n_flat_shapes++] = label_shp;
        }
        return cur_n_flat_shapes;
}

static size_t trip_to_stops_shapes(struct trip *trip,
                                   struct hash_map *seen_stops, double lat_ref,
                                   struct flat_shape *flat_shapes,
                                   size_t n_flat_shapes)
{
        size_t cur_n_flat_shapes = n_flat_shapes;
        for (size_t i = 0; i < trip->stop_time_count; i++) {
                struct stop_time *st = trip->stop_times[i];
                if (!st)
                        continue;
                struct stop *ps = st->stop->parent_station ?
                                          st->stop->parent_station :
                                          st->stop;
                if (!ps || hash_map_get(seen_stops, ps->stop_id))
                        continue;
                uintptr_t mark = 1;
                hash_map_insert(seen_stops, ps->stop_id, (void *)mark);
                cur_n_flat_shapes = stop_to_flat_shape(
                        ps, flat_shapes, cur_n_flat_shapes, lat_ref);

                for (size_t j = 0; j < ps->n_children; j++) {
                        struct stop *s = ps->child_stops[j];

                        cur_n_flat_shapes = stop_to_flat_shape(
                                s, flat_shapes, cur_n_flat_shapes, lat_ref);
                }
        }
        return cur_n_flat_shapes;
}

struct flat_shape *flat_shapes_from_trips(struct trip **trips, size_t n_trips,
                                          struct hash_map *shape_map,
                                          size_t n_shapes,
                                          size_t *n_flat_shapes, size_t n_stops,
                                          double lat_ref)
{
        if (trips == NULL || n_trips == 0)
                return NULL;

        struct flat_shape *flat_shapes = calloc(
                n_shapes + (n_stops * 2),
                sizeof(struct flat_shape)); // Worst case: n shapes, n stops * (1 circle + 1 label)
        *n_flat_shapes = 0;

        struct hash_map *seen_shapes = init_hashmap(n_trips);
        struct hash_map *seen_stops = init_hashmap(n_stops);
        for (size_t i = 0; i < n_trips; i++) {
                struct trip *t = trips[i];
                if (t->shape_id == NULL)
                        continue;

                uintptr_t mark = 1;
                if (!((uintptr_t)hash_map_get(seen_shapes, t->shape_id) == 1)) {
                        hash_map_insert(seen_shapes, t->shape_id, (void *)mark);
                        struct flat_shape line_shape;
                        if (trip_to_line_shape(t, shape_map, lat_ref,
                                               &line_shape))
                                flat_shapes[(*n_flat_shapes)++] = line_shape;
                }
                *n_flat_shapes = trip_to_stops_shapes(
                        t, seen_stops, lat_ref, flat_shapes, *n_flat_shapes);
        }
        flat_shapes = realloc(flat_shapes,
                              *n_flat_shapes * sizeof(struct flat_shape));

        free_hashmap(seen_shapes);
        free_hashmap(seen_stops);
        return flat_shapes;
}

struct flat_shape *flat_shapes_from_itinerary(struct trip **trips, size_t n_trips,
                                        struct stop* dep,
                                        struct stop* arr,
                                          struct hash_map *shape_map,
                                          size_t n_shapes,
                                          size_t *n_flat_shapes, size_t n_stops,
                                          double lat_ref)
{
    if (trips == NULL || n_trips == 0)
            return NULL;

    struct flat_shape *flat_shapes = calloc(
            n_shapes + (n_stops * 2),
            sizeof(struct flat_shape)); // Worst case: n shapes, n stops * (1 circle + 1 label)
    *n_flat_shapes = 0;

    struct hash_map *seen_shapes = init_hashmap(n_trips);
    struct hash_map *seen_stops = init_hashmap(n_stops);
    for (size_t i = 0; i < n_trips; i++) {
            struct trip *t = trips[i];
            if (t->shape_id == NULL)
                    continue;

            uintptr_t mark = 1;
            if (!((uintptr_t)hash_map_get(seen_shapes, t->shape_id) == 1)) {
                    hash_map_insert(seen_shapes, t->shape_id, (void *)mark);
                    struct flat_shape line_shape;
                    if (trip_to_line_shape(t, shape_map, lat_ref,
                                           &line_shape))
                            flat_shapes[(*n_flat_shapes)++] = line_shape;
            }
            *n_flat_shapes = trip_to_stops_shapes(
                    t, seen_stops, lat_ref, flat_shapes, *n_flat_shapes);
    }
    flat_shapes = realloc(flat_shapes,
                          *n_flat_shapes * sizeof(struct flat_shape));

    free_hashmap(seen_shapes);
    free_hashmap(seen_stops);
    return flat_shapes;
}

void free_flat_shapes(struct flat_shape *shapes, size_t n_shapes)
{
        if (shapes == NULL)
                return;
        for (size_t i = 0; i < n_shapes; i++) {
                if (shapes[i].type == LINE)
                        free(shapes[i].as.line.pts);
                else if (shapes[i].type == TEXT)
                        free(shapes[i].as.text.text);
        }
        free(shapes);
}
