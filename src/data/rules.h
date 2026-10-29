#ifndef RULES_H
#define RULES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct route route;
typedef struct stop stop;

enum route_mode {
        BUS,
        METRO,
        TRAMWAY,
        CABLE,
        COMMUTER_RAIL,
        REGIONAL_RAIL,
        RAIL,
        HS_RAIL,
        AIRPORT_RAIL,
        FUNICULAR,
        TAXI,
        UNKNOWN
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
