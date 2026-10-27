#ifndef MAP_TRIP_CUT_H
#define MAP_TRIP_CUT_H

#include "map/map.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool cut_trip_flatshape(struct flat_shape shp, struct stop *dep,
                        struct stop *arr, double lat_ref,
                        struct flat_shape *new_shp);

#endif
