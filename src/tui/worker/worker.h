#ifndef TUI_WORKER_H
#define TUI_WORKER_H

typedef struct worker_data {
        // Agencies
        size_t n_agencies;
        struct agency **agencies;
        struct hash_map *agency_map;

        // Routes
        size_t n_routes;
        struct route **routes;
        struct hash_map *route_map;

        // Trips
        size_t n_trips;
        struct trip **trips;
        struct hash_map *trip_map;

        // Stops
        size_t n_stops;
        struct stop **stops;
        struct hash_map *stop_map;

        // Transfers
        size_t n_transfers;
        struct transfer **transfers;
        struct hash_map *transfers_map;

        // Pathways
        size_t n_pathways;
        struct pathway **pathways;

        // Stop times
        size_t n_stop_times;
        struct stop_time **stop_times;

        // Raptor data
        size_t n_routes_built;
        struct raptor_route **raptor_routes;

        size_t n_stops_built;
        struct raptor_stop **raptor_stops;
        struct hash_map *raptor_stop_map;
} worker_data;

typedef struct worker_agrs {
        int pipe_logic_fd; //Receive msg
        int pipe_ui_fd; //Send messages to ui
} worker_agrs;

void *logic_worker(void *arg);

#endif
