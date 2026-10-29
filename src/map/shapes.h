#ifndef MAP_SHAPES_H
#define MAP_SHAPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "data/gtfs.h"
#include "algo/raptor.h"
#include "map/map.h"
#include "map/style.h"

typedef struct shape gfts_shp;

enum shape_type { LINE, CIRCLE, RECTANGLE, TEXT };

struct line_shape {
        struct flat_pt *pts;
        size_t n_pts;
};

struct circle_shape {
        double radius; //in cells
        double mx;
        double my;
};

struct rectangle_shape {
        double xmax;
        double ymax;

        double xmin;
        double ymin;
};

struct text_shape {
        char *text;
        double mx;
        double my;
};

struct flat_shape {
        enum shape_type type;
        union {
                struct rectangle_shape rect;
                struct line_shape line;
                struct circle_shape circle;
                struct text_shape text;
        } as;
        struct bbox bbox;
        uint32_t color;

        double min_scale;
        double max_scale;
        double z_index;
};

struct flat_shape gtfs_shape_to_flat(gfts_shp *shape, double lat_ref,
                                     struct flat_pt **out, size_t *n_out,
                                     struct shape_style style);
struct flat_shape *gtfs_pathways_to_flat(struct pathway **pathways,
                                         size_t n_pathways, double lat_ref,
                                         size_t *n_shapes);
struct flat_shape *flat_shapes_from_trips(struct trip **trips, size_t n_trips,
                                          struct hash_map *shape_map,
                                          size_t n_shapes,
                                          size_t *n_flat_shapes, size_t n_stops,
                                          double lat_ref);
struct flat_shape *flat_shapes_from_itinerary(
        struct raptor_itinerary *itinerary, struct hash_map *shape_map,
        size_t n_shapes, size_t *n_flat_shapes, size_t n_stops, double lat_ref);

void free_flat_shapes(struct flat_shape *shapes, size_t n_shapes);

#endif
