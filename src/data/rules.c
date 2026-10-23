#include "data/gtfs.h"
#include <stddef.h>
#include <string.h>
#include "data/rules.h"

static enum route_mode route_type_to_mode(unsigned short rt)
{
        if (rt >= 100 && rt <= 117) {
                switch (rt) {
                case 101:
                        return HS_RAIL;
                case 106:
                        return REGIONAL_RAIL;
                case 108:
                        return AIRPORT_RAIL;
                case 109:
                        return COMMUTER_RAIL;
                default:
                        return RAIL;
                }
        }
        if (rt >= 200 && rt <= 209)
                return BUS;
        if (rt >= 400 && rt <= 405)
                return METRO;
        if (rt >= 700 && rt <= 716)
                return BUS;
        if (rt >= 900 && rt <= 906)
                return TRAMWAY;
        if (rt >= 1300 && rt <= 1307)
                return CABLE;
        if (rt >= 1500 && rt <= 1507)
                return TAXI;
        if (rt >= 1700 && rt <= 1702)
                return UNKNOWN;

        switch (rt) {
        case 0:
                return TRAMWAY;
        case 1:
                return METRO;
        case 2:
                return RAIL;
        case 3:
                return BUS;
        case 5:
                return CABLE;
        case 6:
                return CABLE;
        case 7:
                return FUNICULAR;
        case 11:
                return BUS;
        case 12:
                return METRO;
        case 800:
                return BUS;
        default:
                return UNKNOWN;
        }
}

void gtfs_apply_rules(struct route **routes, size_t n_routes)
{
        if (!routes || n_routes == 0)
                return;
        for (size_t i = 0; i < n_routes; i++) {
                struct route *r = routes[i];
                if (!r)
                        continue;
                r->mode = route_type_to_mode(r->route_type);
        }
}