#ifndef MAP_GEO_H
#define MAP_GEO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "data/gtfs.h"
#include "map/map.h"
#include "map/shapes.h"

typedef struct shape gfts_shp;

struct flat_pt latlon_to_flat(double lat, double lon, double lat_ref);
struct flat_pt geo_to_flat(struct geo_pt geo_pt, double lat_ref);

struct cell_pos project_point(struct flat_pt point, struct map_view *map_view);
struct flat_pt unproject_point(struct cell_pos point,
                               struct map_view *map_view);

bool compute_liang_barksy_line(struct bbox screen_box, struct flat_pt pt1,
                               struct flat_pt pt2, struct flat_pt *npt1,
                               struct flat_pt *npt2);

bool check_bbox_intersect(struct bbox box1, struct bbox box2);

struct bbox map_view_bbox(struct map_view *view);
struct bbox get_bbox(struct flat_pt pt1, struct flat_pt pt2);

bool clip_shape(struct flat_shape *shape, struct flat_shape *out_shape,
                struct map_view *view);
struct bbox get_shape_bbox(struct flat_shape *shape);

#endif
