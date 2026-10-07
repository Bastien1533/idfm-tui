#ifndef RAPTOR_H
#define RAPTOR_H

#include "data/gtfs.h"

struct raptor_route_stop {
        size_t pos_in_route;
        struct stop *stop;
        struct raptor_stop *rstop;
        struct raptor_route *route;
};

struct raptor_route {
        struct route *route;

        struct raptor_route_stop **route_stops;
        struct stop_time ***stop_times;

        size_t n_trips;
        size_t n_stops;

        size_t idx;
};

struct raptor_stop {
        struct stop *stop;
        struct transfer **transfers;
        struct stop **childrens;
        struct raptor_route_stop **routes_stops;

        size_t n_transfers;
        size_t n_routes;
        size_t n_chidrens;

        size_t idx;
};

struct raptor_leg {
        struct raptor_stop *from;
        struct raptor_stop *to;
        struct trip *trip; // NULL if transfer/walk
        struct transfer *transfer; // NULL if trip
        unsigned long dep_time;
        unsigned long arr_time;
};
struct raptor_itinerary {
        struct raptor_leg **legs;
        size_t n_legs;
        size_t n_trips;
        size_t n_transfers;
        unsigned long dep_time;
        unsigned long arr_time;
};

struct raptor_result {
        struct raptor_stop ***parent_stops;
        struct trip ***parent_trips;
        struct transfer ***parent_transfers;
        unsigned long int *tau_star;
        unsigned long int **tau_rounds;
        size_t total_rounds;

        struct raptor_stop *dep;
        struct raptor_stop *arr;
        size_t departure_time;

        struct raptor_itinerary **itineraries;
        size_t n_itineraries;
};

unsigned long dep(struct trip *t, struct stop *p);
unsigned long arr(struct trip *t, struct stop *p);

struct raptor_route **build_raptor_routes(struct trip **trips,
                                          const size_t n_trips,
                                          size_t *n_routes);
struct raptor_stop **build_raptor_stops(struct stop **all_stops,
                                        const size_t n_all_stops,
                                        struct raptor_route **routes,
                                        const size_t n_routes,
                                        size_t *n_raptor_stops);
struct hash_map *raptor_stops_to_map(struct raptor_stop **stops,
                                     size_t *n_raptor_stops);

struct raptor_result *
raptor_algo(const size_t n_routes, struct raptor_stop **stops,
            const size_t n_stops, const unsigned long int dep_time,
            struct raptor_stop *dep_stop, struct raptor_stop *arr_stop);

void format_raptor(struct raptor_result *res);

void free_raptor_data(struct raptor_route **raptor_routes,
                      size_t n_raptor_routes, struct raptor_stop **raptor_stops,
                      size_t n_raptor_stops,
                      struct raptor_result *raptor_result);
void free_raptor_itinerary(struct raptor_itinerary *itinerary);
void free_raptor_itineraries(struct raptor_itinerary **itineraries,
                             size_t n_itineraries);
void free_raptor_result(struct raptor_result *raptor_result,
                        bool with_itineraries);

#endif
