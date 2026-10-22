#include "map/map.h"
#include "utils/csv.h"
#include "data/dataset.h"
#include "data/gtfs.h"
#include "algo/raptor.h"
#include "data/transfers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tui/tui.h"

void routes_demo(void)
{
        csv_file *csv_a = read_csv_file("data/gtfs/agency.txt", ',');
        size_t n_agencies = 0;
        struct agency **agencies = get_agencies_from_csv(csv_a, &n_agencies);
        struct hash_map *agency_map = agencies_to_map(agencies, &n_agencies);

        csv_file *csv = read_csv_file("data/gtfs/routes.txt", ',');
        size_t n_routes = 0;
        struct route **routes = get_routes_from_csv(csv, &n_routes, agency_map);
        struct hash_map *route_map = routes_to_map(routes, &n_routes);
        debug_hashmap(route_map);
}

void attributions_demo(void)
{
        csv_file *csv_a = read_csv_file("data/gtfs/agency.txt", ',');
        size_t n_agencies = 0;
        struct agency **agencies = get_agencies_from_csv(csv_a, &n_agencies);
        struct hash_map *agency_map = agencies_to_map(agencies, &n_agencies);

        csv_file *csv = read_csv_file("data/gtfs/routes.txt", ',');
        size_t n_routes = 0;
        struct route **routes = get_routes_from_csv(csv, &n_routes, agency_map);
        struct hash_map *route_map = routes_to_map(routes, &n_routes);

        csv_file *csv_attr = read_csv_file("data/gtfs/attributions.txt", ',');
        size_t n_attributions = 0;
        struct attribution **attributions =
                get_attributions_from_csv(csv_attr, &n_attributions, route_map);
        struct hash_map *attribution_map =
                attributions_to_map(attributions, &n_attributions);
        debug_hashmap(attribution_map);
        struct attribution *attr = hash_map_get(attribution_map, "IDFM:C02706");
        printf("%s", attr->route->route_short_name);
}

void agencies_demo(void)
{
        const char *filepath = "data/gtfs/agency.txt";

        printf("Reading %s...\n", filepath);
        csv_file *csv = read_csv_file(filepath, ',');
        size_t n_agencies = 0;
        struct agency **agencies = get_agencies_from_csv(csv, &n_agencies);
        printf("%zu\n", n_agencies);
        for (size_t i = 0; i < n_agencies; i++) {
                struct agency *a = agencies[i];
                printf("%s\n", a->agency_name);
        }

        struct hash_map *agency_map = agencies_to_map(agencies, &n_agencies);
        debug_hashmap(agency_map);

        struct agency *ratp = hash_map_get(agency_map, "IDFM:Operator_100");
        printf("Test RATP : [%s] [%s]", ratp->agency_id, ratp->agency_name);
}

void stops_demo(void)
{
        const char *filepath = "data/gtfs/stops.txt";

        printf("Reading %s...\n", filepath);
        csv_file *csv = read_csv_file(filepath, ',');
        size_t n_stops = 0;
        struct stop **stops = get_stops_from_csv(csv, &n_stops);
        printf("%zu\n", n_stops);

        struct hash_map *stop_map = stops_to_map(stops, &n_stops);
        debug_hashmap(stop_map);
}

void tranfers_demo(void)
{
        const char *filepath = "data/gtfs/stops.txt";

        printf("Reading %s...\n", filepath);
        csv_file *csv = read_csv_file(filepath, ',');
        size_t n_stops = 0;
        struct stop **stops = get_stops_from_csv(csv, &n_stops);
        struct hash_map *stop_map = stops_to_map(stops, &n_stops);

        filepath = "data/gtfs/transfers.txt";
        printf("Reading %s...\n", filepath);
        csv_file *csv_t = read_csv_file(filepath, ',');
        size_t n_transfers = 0;
        struct transfer **transfers =
                get_transfers_from_csv(csv_t, &n_transfers, stop_map);
        printf("%zu\n", n_transfers);

        struct hash_map *transfers_map =
                transfers_to_map(transfers, &n_transfers);
        debug_hashmap(transfers_map);
}

void trips_demo(void)
{
        csv_file *csv_a = read_csv_file("data/gtfs/agency.txt", ',');
        size_t n_agencies = 0;
        struct agency **agencies = get_agencies_from_csv(csv_a, &n_agencies);
        struct hash_map *agency_map = agencies_to_map(agencies, &n_agencies);

        csv_file *csv_r = read_csv_file("data/gtfs/routes.txt", ',');
        size_t n_routes = 0;
        struct route **routes =
                get_routes_from_csv(csv_r, &n_routes, agency_map);
        struct hash_map *route_map = routes_to_map(routes, &n_routes);

        csv_file *csv_t = read_csv_file("data/gtfs/trips.txt", ',');
        size_t n_trips = 0;
        struct trip **trips = get_trips_from_csv(csv_t, &n_trips, route_map);
        size_t map_capacity = n_trips * 4;
        printf("%zu", map_capacity);
        struct hash_map *trip_map = trips_to_map(trips, &n_trips, map_capacity);
}

void stop_times_demo(void)
{
        /* --- agencies -> routes --- */
        csv_file *csv_a = read_csv_file("data/gtfs/agency.txt", ',');
        size_t n_agencies = 0;
        struct agency **agencies = get_agencies_from_csv(csv_a, &n_agencies);
        struct hash_map *agency_map = agencies_to_map(agencies, &n_agencies);

        csv_file *csv_r = read_csv_file("data/gtfs/routes.txt", ',');
        size_t n_routes = 0;
        struct route **routes =
                get_routes_from_csv(csv_r, &n_routes, agency_map);
        struct hash_map *route_map = routes_to_map(routes, &n_routes);

        /* --- trips -> trip_map --- */
        csv_file *csv_t = read_csv_file("data/gtfs/trips.txt", ',');
        size_t n_trips = 0;
        struct trip **trips = get_trips_from_csv(csv_t, &n_trips, route_map);
        struct hash_map *trip_map = trips_to_map(trips, &n_trips, n_trips * 4);
        printf("trips: %zu\n", n_trips);

        /* --- stops -> stop_map --- */
        printf("Reading %s...\n", "data/gtfs/stops.txt");
        csv_file *csv_s = read_csv_file("data/gtfs/stops.txt", ',');
        size_t n_stops = 0;
        struct stop **stops = get_stops_from_csv(csv_s, &n_stops);
        struct hash_map *stop_map = stops_to_map(stops, &n_stops);

        /* --- transfers --- */
        printf("Reading %s...\n", "data/gtfs/transfers.txt");
        csv_file *csv_x = read_csv_file("data/gtfs/transfers.txt", ',');
        size_t n_transfers = 0;
        struct transfer **transfers =
                get_transfers_from_csv(csv_x, &n_transfers, stop_map);
        printf("transfers: %zu\n", n_transfers);

        /* --- stop_times --- */
        printf("Reading %s...\n", "data/gtfs/stop_times.txt");
        csv_file *csv_st = read_csv_file("data/gtfs/stop_times.txt", ',');
        size_t n_stop_times = 0;
        struct stop_time **stop_times = get_stop_times_from_csv(
                csv_st, &n_stop_times, trip_map, stop_map);
        printf("stop_times: %zu\n", n_stop_times);

        /* --- probe one trip --- */
        struct trip *temp = hash_map_get(
                trip_map,
                "IDFM:RATP:COU_RATP_05JODIAMANT1_2026_915658782_RQue02Bobi25-2");
        if (temp) {
                for (size_t i = 0; i < temp->stop_time_count; i++) {
                        printf("%s %zu\n", temp->stop_times[i]->stop->stop_name,
                               temp->stop_times[i]->stop_sequence);
                }
        }

        /* --- raptor build (NOTE: header declares 3-arg sig) --- */
        size_t n_routes_built = 0;
        struct raptor_route **raptor_routes =
                build_raptor_routes(trips, n_trips, &n_routes_built);
        printf("raptor routes built: %zu\n", n_routes_built);

        size_t n_stops_built = 0;
        struct raptor_stop **raptor_stops = build_raptor_stops(
                stops, n_stops, raptor_routes, n_routes_built, &n_stops_built);
        printf("raptor stops built: %zu\n", n_stops_built);

        struct hash_map *raptor_stop_map =
                raptor_stops_to_map(raptor_stops, &n_stops_built);
        struct raptor_stop *dep = hash_map_get(raptor_stop_map, "IDFM:462981");
        struct raptor_stop *arr = hash_map_get(raptor_stop_map, "IDFM:490916");
        printf("%s\n", dep->stop->stop_name);
        struct raptor_result *res = raptor_algo(n_routes_built, raptor_stops,
                                                n_stops_built, 8 * 3600, dep,
                                                arr);

        /* --- free everything --- */
        free_hashmap(raptor_stop_map);
        free_hashmap(route_map);
        free_hashmap(trip_map);
        free_hashmap(stop_map);
        free_hashmap(agency_map);

        free_raptor_data(raptor_routes, n_routes_built, raptor_stops,
                         n_stops_built, res);
        free_stop_times(stop_times, n_stop_times);
        free_transfers(transfers, n_transfers);
        free_trips(trips, n_trips);
        free_stops(stops, n_stops);
        free_routes(routes, n_routes);
        free_agencies(agencies, n_agencies);
}

void pathways_demo(void)
{
        /* --- agencies -> routes --- */
        csv_file *csv_a = read_csv_file("data/gtfs2/agency.txt", ',');
        size_t n_agencies = 0;
        struct agency **agencies = get_agencies_from_csv(csv_a, &n_agencies);
        struct hash_map *agency_map = agencies_to_map(agencies, &n_agencies);

        csv_file *csv_r = read_csv_file("data/gtfs2/routes.txt", ',');
        size_t n_routes = 0;
        struct route **routes =
                get_routes_from_csv(csv_r, &n_routes, agency_map);
        struct hash_map *route_map = routes_to_map(routes, &n_routes);

        /* --- trips -> trip_map --- */
        csv_file *csv_t = read_csv_file("data/gtfs2/trips.txt", ',');
        size_t n_trips = 0;
        struct trip **trips = get_trips_from_csv(csv_t, &n_trips, route_map);
        struct hash_map *trip_map = trips_to_map(trips, &n_trips, n_trips * 4);
        printf("trips: %zu\n", n_trips);

        /* --- stops -> stop_map --- */
        printf("Reading %s...\n", "data/gtfs2/stops.txt");
        csv_file *csv_s = read_csv_file("data/gtfs2/stops.txt", ',');
        size_t n_stops = 0;
        struct stop **stops = get_stops_from_csv(csv_s, &n_stops);
        struct hash_map *stop_map = stops_to_map(stops, &n_stops);

        /* --- dataset_stops -> dataset_stop_map --- */
        printf("Reading %s...\n", "data/arrets-ligne.csv");
        csv_file *csv_ds = read_csv_file("data/arrets-ligne.csv", ';');
        size_t n_dstops = 0;
        struct dataset_stop **d_stops =
                get_dataset_stops_from_csv(csv_ds, &n_dstops);
        struct hash_map *dataset_stop_map =
                dataset_stops_to_map(d_stops, &n_dstops);

        /* --- transfers --- */
        printf("Reading %s...\n", "data/gtfs2/transfers.txt");
        csv_file *csv_x = read_csv_file("data/gtfs2/transfers.txt", ',');
        size_t n_transfers = 0;
        struct transfer **transfers =
                get_transfers_from_csv(csv_x, &n_transfers, stop_map);
        printf("transfers: %zu\n", n_transfers);

        /* --- pathways --- */
        printf("Reading %s...\n", "data/gtfs2/pathways.txt");
        csv_file *csv_p = read_csv_file("data/gtfs2/pathways.txt", ',');
        size_t n_pathways = 0;
        struct pathway **pathways =
                get_pathways_from_csv(csv_p, &n_pathways, stop_map);
        printf("pathways: %zu\n", n_pathways);

        update_transfers_from_pathways(
                pathways, n_pathways, stops, n_stops, transfers, n_transfers,
                transfers_to_map(transfers, &n_transfers), dataset_stop_map);

        /* --- free everything --- */
        free_hashmap(route_map);
        free_hashmap(trip_map);
        free_hashmap(stop_map);
        free_hashmap(agency_map);

        free_transfers(transfers, n_transfers);
        free_trips(trips, n_trips);
        free_stops(stops, n_stops);
        free_routes(routes, n_routes);
        free_agencies(agencies, n_agencies);
}

void raptor_demo(void)
{
        /* --- agencies -> routes --- */
        printf("Reading %s...\n", "data/gtfs2/agency.txt");
        csv_file *csv_a = read_csv_file("data/gtfs2/agency.txt", ',');
        size_t n_agencies = 0;
        struct agency **agencies = get_agencies_from_csv(csv_a, &n_agencies);
        struct hash_map *agency_map = agencies_to_map(agencies, &n_agencies);

        printf("Reading %s...\n", "data/gtfs2/routes.txt");
        csv_file *csv_r = read_csv_file("data/gtfs2/routes.txt", ',');
        size_t n_routes = 0;
        struct route **routes =
                get_routes_from_csv(csv_r, &n_routes, agency_map);
        struct hash_map *route_map = routes_to_map(routes, &n_routes);

        /* --- trips -> trip_map --- */
        printf("Reading %s...\n", "data/gtfs2/trips.txt");
        csv_file *csv_t = read_csv_file("data/gtfs2/trips.txt", ',');
        size_t n_trips = 0;
        struct trip **trips = get_trips_from_csv(csv_t, &n_trips, route_map);
        struct hash_map *trip_map = trips_to_map(trips, &n_trips, n_trips * 4);
        printf("trips: %zu\n", n_trips);

        /* --- stops -> stop_map --- */
        printf("Reading %s...\n", "data/gtfs2/stops.txt");
        csv_file *csv_s = read_csv_file("data/gtfs2/stops.txt", ',');
        size_t n_stops = 0;
        struct stop **stops = get_stops_from_csv(csv_s, &n_stops);
        struct hash_map *stop_map = stops_to_map(stops, &n_stops);

        /* --- dataset_stops -> dataset_stop_map --- */
        printf("Reading %s...\n", "data/arrets-ligne.csv");
        csv_file *csv_ds = read_csv_file("data/arrets-ligne.csv", ';');
        size_t n_dstops = 0;
        struct dataset_stop **d_stops =
                get_dataset_stops_from_csv(csv_ds, &n_dstops);
        struct hash_map *dataset_stop_map =
                dataset_stops_to_map(d_stops, &n_dstops);

        /* --- transfers --- */
        printf("Reading %s...\n", "data/gtfs2/transfers.txt");
        csv_file *csv_x = read_csv_file("data/gtfs2/transfers.txt", ',');
        size_t n_transfers = 0;
        struct transfer **transfers =
                get_transfers_from_csv(csv_x, &n_transfers, stop_map);
        struct hash_map *transfers_map =
                transfers_to_map(transfers, &n_transfers);
        printf("transfers: %zu\n", n_transfers);

        link_parent_stops(stops, n_stops, stop_map, &transfers, &n_transfers,
                          transfers_map);

        /* --- pathways -> populate transfer steps --- */
        printf("Reading %s...\n", "data/gtfs2/pathways.txt");
        csv_file *csv_p = read_csv_file("data/gtfs2/pathways.txt", ',');
        size_t n_pathways = 0;
        struct pathway **pathways =
                get_pathways_from_csv(csv_p, &n_pathways, stop_map);
        printf("pathways: %zu\n", n_pathways);

        /*update_transfers_from_pathways(pathways, n_pathways, stops, n_stops,
                                       transfers, n_transfers, transfers_map,
                                       dataset_stop_map); */

        /* --- stop_times --- */
        printf("Reading %s...\n", "data/gtfs2/stop_times.txt");
        csv_file *csv_st = read_csv_file("data/gtfs2/stop_times.txt", ',');
        size_t n_stop_times = 0;
        struct stop_time **stop_times = get_stop_times_from_csv(
                csv_st, &n_stop_times, trip_map, stop_map);
        printf("stop_times: %zu\n", n_stop_times);

        /* --- raptor build + itinerary --- */
        size_t n_routes_built = 0;
        struct raptor_route **raptor_routes =
                build_raptor_routes(trips, n_trips, &n_routes_built);
        printf("raptor routes built: %zu\n", n_routes_built);

        size_t n_stops_built = 0;
        struct raptor_stop **raptor_stops = build_raptor_stops(
                stops, n_stops, raptor_routes, n_routes_built, &n_stops_built);
        printf("raptor stops built: %zu\n", n_stops_built);

        struct hash_map *raptor_stop_map =
                raptor_stops_to_map(raptor_stops, &n_stops_built);
        printf("raptor stop map built\n");
        for (size_t i = 0; i < n_stops_built; i++) {
                printf("test %s\n", raptor_stops[i]->stop->stop_name);
        }
        printf("fnished\n");
        for (size_t i = 0; i < n_stops_built; i++) {
                struct raptor_stop *test = raptor_stops[i];
                if (strcmp(test->stop->stop_id, "IDFM:60915") == 0) {
                        printf("test %s\n", test->stop->stop_name);
                }
        }
        printf("end of test");

        struct raptor_stop *arr = hash_map_get(raptor_stop_map, "IDFM:73619");
        struct raptor_stop *dep = hash_map_get(raptor_stop_map, "IDFM:71920");
        struct raptor_result *res = NULL;
        if (dep && arr) {
                printf("%s -> %s\n", dep->stop->stop_name,
                       arr->stop->stop_name);
                res = raptor_algo(n_routes_built, raptor_stops, n_stops_built,
                                  8 * 3600, dep, arr);
                format_raptor(res);
        }

        /* --- free everything --- */
        free_hashmap(raptor_stop_map);
        free_hashmap(transfers_map);
        free_hashmap(dataset_stop_map);
        free_hashmap(route_map);
        free_hashmap(trip_map);
        free_hashmap(stop_map);
        free_hashmap(agency_map);

        free_raptor_data(raptor_routes, n_routes_built, raptor_stops,
                         n_stops_built, res);
        free_stop_times(stop_times, n_stop_times);
        free_pathways(pathways, n_pathways);
        free_transfers(transfers, n_transfers);
        free_dataset_stops(
                d_stops,
                n_dstops); /* NOTE: known bug inside (stops[1] index) — fix separately */
        free_trips(trips, n_trips);
        free_stops(stops, n_stops);
        free_routes(routes, n_routes);
        free_agencies(agencies, n_agencies);

        free_csv(csv_a);
        free_csv(csv_r);
        free_csv(csv_t);
        free_csv(csv_s);
        free_csv(csv_ds);
        free_csv(csv_x);
        free_csv(csv_p);
        free_csv(csv_st);
}

int main(void)
{
        render();
        return 0;
}
