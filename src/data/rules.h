#ifndef RULES_H
#define RULES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct route route;
typedef struct stop stop;

enum route_mode {
        BUS = 1 << 0,
        METRO = 1 << 1,
        TRAMWAY = 1 << 2,
        CABLE = 1 << 3,
        COMMUTER_RAIL = 1 << 4,
        REGIONAL_RAIL = 1 << 5,
        RAIL = 1 << 6,
        HS_RAIL = 1 << 7,
        AIRPORT_RAIL = 1 << 8,
        FUNICULAR = 1 << 9,
        TAXI = 1 << 10,
        UNKNOWN = 1 << 11,
};

enum location_type {
        PLATFORM = 0,
        STATION = 1,
        ENTRANCE_EXIT = 2,
        GENERIC = 3,
        BOARDING_AREA = 4,
};

enum stop_role {
        STOP_ROLE_DEFAULT = 0,
        STOP_ROLE_DEP = 1,
        STOP_ROLE_ARR = 2,
        STOP_ROLE_TRANSFER = 3,
        STOP_ROLE_INTERMEDIATE = 4,
};

enum feed_origin { FEED_IDFM, FEED_MTA, FEED_BVG, FEED_GENERIC };

void gtfs_apply_rules(struct route **routes, size_t n_routes);

#endif
