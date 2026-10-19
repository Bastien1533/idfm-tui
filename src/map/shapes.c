#include "data/gtfs.h"
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
                                     struct flat_pt **out, size_t *n_out)
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
                .min_scale = -1,
                .max_scale = -1,
                .color = (uint32_t)((shape->route &&
                                     shape->route->route_color[0]) ?
                                            strtoul(shape->route->route_color,
                                                    NULL, 16) :
                                            0xFFFFFF)
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

struct flat_shape *stops_to_circles(struct stop **stops, size_t n_stops,
                                    size_t *n_shapes, double lat_ref)
{
        if (stops == NULL || n_stops == 0)
                return NULL;

        double xmax = INT_MIN;
        double ymax = INT_MIN;
        double xmin = INT_MAX;
        double ymin = INT_MAX;

        struct flat_shape *out_shapes =
                calloc(n_stops * n_stops, sizeof(struct flat_shape));
        if (out_shapes == NULL)
                return NULL;

        for (size_t i = 0; i < n_stops; i++) {
                struct stop *stop = stops[i];
                struct stop *parent_stop = stop->parent_station == NULL ?
                                                   stop :
                                                   stop->parent_station;
                if (stop == NULL || stop->location_type == 0)
                        continue;
                bool has_tram = 0;
                bool has_subway = 0;
                bool has_rer = 0;
                for (size_t j = 0; j < parent_stop->n_assoc_routes; j++) {
                        if (parent_stop->assoc_routes[j]->route_type == 0)
                                has_tram = 1;
                        if (parent_stop->assoc_routes[j]->route_type == 1)
                                has_subway = 1;
                        if (parent_stop->assoc_routes[j]->route_type == 2)
                                has_rer = 1;
                }
                if (has_rer == 0 && has_subway == 0 && has_tram == 0)
                        continue;

                struct flat_pt fp =
                        latlon_to_flat(stop->stop_lat, stop->stop_lon, lat_ref);

                xmax = fmax(xmax, fp.x);
                ymax = fmax(ymax, fp.y);
                xmin = fmin(xmin, fp.x);
                ymin = fmin(ymin, fp.y);

                uint32_t color = stop->location_type == 1 ?
                                         0xFFFFF :
                                         (stop->location_type == 0 ? 0xFFFFFF :
                                                                     0x7A7A7A);

                struct flat_shape shape = {
                    .type = CIRCLE,
                    .bbox = {
                        .xmax = xmax,
                        .xmin = xmin,
                        .ymax = ymax,
                        .ymin = ymin,
                    },
                    .color = color,
                    .as.circle = {
                        .radius = stop->location_type == 1 ? 2 : (stop->location_type == 0 ? 1 : 1),
                        .mx = fp.x,
                        .my = fp.y,
                    },
                    .min_scale = stop->location_type == 1 ? 20 : (stop->location_type == 0 ? 300 : 300),
                    .max_scale = -1,
                };

                out_shapes[*n_shapes] = shape;
                (*n_shapes)++;

                if (stop->location_type == 1) {
                        struct flat_shape shape = {
                        .type = TEXT,
                        .bbox = {
                            .xmax = xmax,
                            .xmin = xmin,
                            .ymax = ymax,
                            .ymin = ymin,
                        },
                        .color = 0xFFFFF,
                        .as.text.text = strdup(stop->stop_name),
                        .as.text.mx = fp.x,
                        .as.text.my = fp.y,
                        .min_scale = 50,
                        .max_scale = -1,
                    };

                        out_shapes[*n_shapes] = shape;
                        (*n_shapes)++;
                }
                if (stop->location_type == 2) {
                        struct flat_shape shape = {
                        .type = TEXT,
                        .bbox = {
                            .xmax = xmax,
                            .xmin = xmin,
                            .ymax = ymax,
                            .ymin = ymin,
                        },
                        .color = 0x7A7A7A,
                        .as.text.text = strdup(stop->stop_name),
                        .as.text.mx = fp.x,
                        .as.text.my = fp.y,
                        .min_scale = 300,
                        .max_scale = -1,
                    };

                        out_shapes[*n_shapes] = shape;
                        (*n_shapes)++;
                }
        }
        struct flat_shape *out_shapes_t =
                realloc(out_shapes, *n_shapes * sizeof(struct flat_shape));
        if (out_shapes_t == NULL)
                return NULL;
        out_shapes = out_shapes_t;
        return out_shapes;
}
