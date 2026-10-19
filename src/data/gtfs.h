#ifndef GTFS_H
#define GTFS_H

#include "utils/csv.h"
#include "utils/hashmap.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

struct agency {
        char *agency_id;
        char *agency_name;
        char *agency_url;
        char *agency_timezone;
        char *agency_lang;
        char *agency_phone;
        char *agency_email;
        char *agency_fare_url;
        char *ticketing_deep_link_id;
};

struct route {
        char *route_id;
        struct agency *agency;
        char *route_short_name;
        char *route_long_name;
        char *route_desc;
        unsigned short route_type;
        char *route_url;
        char *route_color;
        char *route_text_color;

        struct hash_map *end_stop_freq;
        struct stop **end_stops;
        size_t n_end_stops;
};

struct stop {
        char *stop_id;
        unsigned short int stop_code;
        char *stop_name;
        char *stop_desc;
        double stop_lon;
        double stop_lat;
        unsigned short int zone_id;
        char *stop_url;
        unsigned short int location_type;

        char *parent_station_id;
        struct stop *parent_station;

        struct stop **child_stops;
        size_t n_children;

        struct route **assoc_routes;
        size_t n_assoc_routes;

        char *stop_timezone;
        char *level_id;
        unsigned short int wheelchair_boarding;
        char *platform_code;

        struct transfer **transfers;
        size_t transfer_count;

        size_t idx;
};

struct attribution {
        char *attribution_id;
        struct route *route;
        char *trip_id;
        unsigned short is_operator;
        char *organization_name;
        char *attribution_url;
        char *attribution_email;
        char *attribution_phone;
};

struct shape_seq {
        double shape_pt_lat;
        double shape_pt_lon;
        unsigned short shape_pt_sequence;
};

struct shape {
        char *shape_id;
        struct shape_seq **points;
        size_t points_n;

        struct route *route;
};

struct trip {
        struct route *route;
        char *service_id;
        char *trip_id;
        char *trip_headsign;
        char *trip_short_name;
        size_t direction_id;
        char *shape_id;

        struct stop_time **stop_times;
        size_t stop_time_count;
};

struct stop_time {
        struct trip *trip;
        struct stop *stop;
        size_t departure_time;
        size_t start_pickup_drop_off_window;
        size_t arrival_time;
        size_t end_pickup_drop_off_window;
        size_t stop_sequence;
        size_t pickup_type;
        size_t drop_off_type;
};

struct transfer {
        struct stop *from_stop;
        struct stop *to_stop;

        struct raptor_stop *to_rstop;

        bool is_inter_parent_transfer;
        unsigned short int transfer_type;
        unsigned short int min_transfer_time;

        struct pathway **steps;
        size_t n_steps;
};

struct pathway {
        char *pathway_id;
        struct stop *from_stop;
        struct stop *to_stop;
        unsigned short int pathway_mode;
        unsigned short int is_bidirectional;
        double length;
        unsigned short int traversal_time;
        short int max_slope;
        short int min_width;
        char *signposted_as;
        char *reversed_signposted_as;
};

struct agency **get_agencies_from_csv(csv_file *csv, size_t *n_agencies);
struct stop **get_stops_from_csv(csv_file *csv, size_t *n_stops);
struct attribution **get_attributions_from_csv(csv_file *csv,
                                               size_t *n_attributions,
                                               struct hash_map *routes);
struct transfer **get_transfers_from_csv(csv_file *csv, size_t *n_transfers,
                                         struct hash_map *stops);
struct pathway **get_pathways_from_csv(csv_file *csv, size_t *n_pathways,
                                       struct hash_map *stops);
struct route **get_routes_from_csv(csv_file *csv, size_t *n_routes,
                                   struct hash_map *agency_map);
struct trip **get_trips_from_csv(csv_file *csv, size_t *n_trips,
                                 struct hash_map *route_map);
struct stop_time **get_stop_times_from_csv(csv_file *csv, size_t *n_stop_times,
                                           struct hash_map *trip_map,
                                           struct hash_map *stop_map);
struct shape **get_shapes_from_csv(csv_file *csv, size_t *n_shapes);
void associate_shape_route(struct trip **trips, size_t n_trips,
                           struct hash_map *shape_map);

void populate_stop_routes(struct stop_time **stop_times, size_t stop_times_n,
                          struct stop **stops, size_t stops_n);
void populate_route_ends(struct trip **trips, size_t n_trips,
                         struct hash_map *stop_map, struct route **routes,
                         size_t n_routes);
void link_parent_stops(struct stop **stops, size_t n_stops,
                       struct hash_map *stop_map, struct transfer ***transfers,
                       size_t *n_transfers, struct hash_map *transfer_map);
struct hash_map *agencies_to_map(struct agency **agencies, size_t *n_agencies);
struct hash_map *stops_to_map(struct stop **stops, size_t *n_stops);
struct hash_map *transfers_to_map(struct transfer **transfers,
                                  size_t *n_transfers);
struct hash_map *pathways_to_map(struct pathway **pathways, size_t *n_pathways);
struct hash_map *shapes_to_map(struct shape **shapes, size_t *n_shapes);
struct hash_map *attributions_to_map(struct attribution **attributions,
                                     size_t *n_attribution);
struct hash_map *routes_to_map(struct route **routes, size_t *n_routes);
struct hash_map *trips_to_map(struct trip **trips, size_t *n_trips,
                              size_t capacity);
struct hash_map *stop_times_to_map(struct stop_time **stop_times,
                                   size_t *n_stop_times);

void free_agencies(struct agency **agencies, size_t n_agencies);
void free_stops(struct stop **stops, size_t n_stops);
void free_transfers(struct transfer **transfers, size_t n_transfers);
void free_pathways(struct pathway **pathways, size_t n_pathways);
void free_attributions(struct attribution **attributions,
                       size_t n_attributions);
void free_routes(struct route **routes, size_t n_routes);
void free_trips(struct trip **trips, size_t n_trips);
void free_stop_times(struct stop_time **stop_times, size_t n_stop_times);

#endif
