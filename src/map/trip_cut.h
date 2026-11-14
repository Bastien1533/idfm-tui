#ifndef MAP_TRIP_CUT_H
#define MAP_TRIP_CUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "map/map.h"

bool cut_trip_flatshape(struct flat_shape shp, struct stop *dep,
                        struct stop *arr, double lat_ref,
                        struct flat_shape *new_shp);

#endif
