#include "data/gtfs.h"
#include "data/transfers.h"

#include "algo/raptor.h"
#include "tui/msg.h"
#include "tui/protocol.h"

#include "tui/worker/worker.h"
#include "tui/worker/search.h"
#include "tui/worker/itinerary.h"
#include "tui/worker/map.h"

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void free_worker_data(worker_data *data)
{
        if (data == NULL) {
                return;
        }
        free_raptor_data(data->raptor_routes, data->n_routes_built,
                         data->raptor_stops, data->n_stops_built, NULL);

        if (data->agencies)
                free_agencies(data->agencies, data->n_agencies);

        if (data->routes)
                free_routes(data->routes, data->n_routes);
        if (data->stops)
                free_stops(data->stops, data->n_stops);
        if (data->trips)
                free_trips(data->trips, data->n_trips);

        if (data->transfers)
                free_transfers(data->transfers, data->n_transfers);

        if (data->pathways)
                free_pathways(data->pathways, data->n_pathways);

        if (data->stop_times)
                free_stop_times(data->stop_times, data->n_stop_times);

        free_hashmap(data->agency_map);
        free_hashmap(data->shape_map);
        free_hashmap(data->stop_map);
        free_hashmap(data->route_map);
        free_hashmap(data->trip_map);
        free_hashmap(data->transfers_map);
        free_hashmap(data->raptor_stop_map);
        free(data);
}

void init_worker(int ui_pipe_fd, worker_data *data)
{
        /* --- agencies -> routes --- */
        send_log_ui_message(ui_pipe_fd, "Reading %s...\n",
                            "data/gtfs2/agency.txt");
        csv_file *csv_a = read_csv_file("data/gtfs2/agency.txt", ',');
        size_t n_agencies = 0;
        data->agencies = get_agencies_from_csv(csv_a, &n_agencies);
        data->agency_map = agencies_to_map(data->agencies, &n_agencies);
        data->n_agencies = n_agencies;

        send_log_ui_message(ui_pipe_fd, "Reading %s...\n",
                            "data/gtfs2/routes.txt");
        csv_file *csv_r = read_csv_file("data/gtfs2/routes.txt", ',');
        size_t n_routes = 0;
        data->routes = get_routes_from_csv(csv_r, &n_routes, data->agency_map);
        data->route_map = routes_to_map(data->routes, &n_routes);
        data->n_routes = n_routes;

        /* --- trips -> trip_map --- */
        send_log_ui_message(ui_pipe_fd, "Reading %s...\n",
                            "data/gtfs2/trips.txt");
        csv_file *csv_t = read_csv_file("data/gtfs2/trips.txt", ',');
        size_t n_trips = 0;
        data->trips = get_trips_from_csv(csv_t, &n_trips, data->route_map);
        data->trip_map = trips_to_map(data->trips, &n_trips, n_trips * 4);
        send_log_ui_message(ui_pipe_fd, "trips: %zu\n", n_trips);
        data->n_trips = n_trips;

        /* --- stops -> stop_map --- */
        send_log_ui_message(ui_pipe_fd, "Reading %s...\n",
                            "data/gtfs2/stops.txt");
        csv_file *csv_s = read_csv_file("data/gtfs2/stops.txt", ',');
        size_t n_stops = 0;
        data->stops = get_stops_from_csv(csv_s, &n_stops);
        data->stop_map = stops_to_map(data->stops, &n_stops);
        data->n_stops = n_stops;

        /* --- transfers --- */
        send_log_ui_message(ui_pipe_fd, "Reading %s...\n",
                            "data/gtfs2/transfers.txt");
        csv_file *csv_x = read_csv_file("data/gtfs2/transfers.txt", ',');
        size_t n_transfers = 0;
        data->transfers =
                get_transfers_from_csv(csv_x, &n_transfers, data->stop_map);
        data->transfers_map = transfers_to_map(data->transfers, &n_transfers);
        data->n_transfers = n_transfers;
        send_log_ui_message(ui_pipe_fd, "Parsed %zu transfers\n", n_transfers);

        link_parent_stops(data->stops, n_stops, data->stop_map,
                          &data->transfers, &n_transfers, data->transfers_map);

        /* --- pathways -> populate transfer steps --- */
        send_log_ui_message(ui_pipe_fd, "Reading %s...\n",
                            "data/gtfs2/pathways.txt");
        csv_file *csv_p = read_csv_file("data/gtfs2/pathways.txt", ',');
        size_t n_pathways = 0;
        data->pathways =
                get_pathways_from_csv(csv_p, &n_pathways, data->stop_map);
        data->n_pathways = n_pathways;
        send_log_ui_message(ui_pipe_fd, "Parsed %zu pathways\n", n_pathways);

        update_transfers_from_pathways(data->pathways, n_pathways, data->stops,
                                       n_stops, data->transfers, n_transfers,
                                       data->transfers_map, data->stop_map);

        /* --- stop_times --- */
        send_log_ui_message(ui_pipe_fd, "Reading %s...\n",
                            "data/gtfs2/stop_times.txt");
        csv_file *csv_st = read_csv_file("data/gtfs2/stop_times.txt", ',');
        size_t n_stop_times = 0;
        data->stop_times = get_stop_times_from_csv(
                csv_st, &n_stop_times, data->trip_map, data->stop_map);
        data->n_stop_times = n_stop_times;
        send_log_ui_message(ui_pipe_fd, "Parsed %zu stop_times\n",
                            n_stop_times);

        populate_stop_routes(data->stop_times, data->n_stop_times, data->stops,
                             data->n_stops);

        populate_route_ends(data->trips, data->n_trips, data->stop_map,
                            data->routes, data->n_routes);
        send_log_ui_message(ui_pipe_fd, "Populated route data\n");

        /* --- shapes --- */
        send_log_ui_message(ui_pipe_fd, "Reading %s...\n",
                            "data/gtfs2/shapes.txt");
        csv_file *csv_sh = read_csv_file("data/gtfs2/shapes.txt", ',');
        size_t n_shapes = 0;
        double lat_ref;
        data->shapes = get_shapes_from_csv(csv_sh, &n_shapes, &lat_ref);
        data->n_shapes = n_shapes;
        data->lat_ref = lat_ref;
        free_csv(csv_sh);
        send_log_ui_message(ui_pipe_fd, "Parsed %zu shapes\n", n_shapes);

        struct hash_map *shape_map = shapes_to_map(data->shapes, &n_shapes);
        data->shape_map = shape_map;
        associate_shape_route(data->trips, n_trips, data->shape_map);
        send_log_ui_message(ui_pipe_fd, "Populated shape routes\n");

        /* --- raptor build + itinerary --- */
        size_t n_routes_built = 0;
        data->raptor_routes =
                build_raptor_routes(data->trips, n_trips, &n_routes_built);
        data->n_routes_built = n_routes_built;
        send_log_ui_message(ui_pipe_fd, "raptor routes built: %zu\n",
                            n_routes_built);

        size_t n_stops_built = 0;
        data->raptor_stops = build_raptor_stops(data->stops, n_stops,
                                                data->raptor_routes,
                                                n_routes_built, &n_stops_built);
        data->n_stops_built = n_stops_built;
        send_log_ui_message(ui_pipe_fd, "raptor stops built: %zu\n",
                            n_stops_built);

        data->raptor_stop_map =
                raptor_stops_to_map(data->raptor_stops, &n_stops_built);
        send_log_ui_message(ui_pipe_fd, "raptor stop map built\n");
        send_init_finished_message(ui_pipe_fd);

        free_csv(csv_a);
        free_csv(csv_r);
        free_csv(csv_t);
        free_csv(csv_s);
        free_csv(csv_x);
        free_csv(csv_p);
        free_csv(csv_st);
}

void *logic_worker(void *arg)
{
        if (arg == NULL)
                return NULL;
        worker_agrs *wargs = (worker_agrs *)arg;

        worker_data *wdata = calloc(1, sizeof(worker_data));
        if (wdata == NULL)
                return NULL;

        logic_task_req *incoming_req;
        bool init = false;
        size_t i = 0;
        while (1) {
                if (read(wargs->pipe_logic_fd, &incoming_req,
                         sizeof(incoming_req)) == sizeof(incoming_req)) {
                        if (incoming_req->type == LOGIC_TASK_INIT_REQ &&
                            !init) {
                                init_worker(wargs->pipe_ui_fd, wdata);
                                init = true;
                        } else if (incoming_req->type == LOGIC_SHUTDOWN) {
                                free_worker_data(wdata);
                                free(incoming_req);
                                break;
                        } else if (incoming_req->type == LOGIC_TASK_STOP_REQ) {
                                struct search_res *res = get_search_stops(
                                        &incoming_req->stop_req, wdata->stops,
                                        wdata->n_stops);
                                if (res != NULL) {
                                        send_search_res_ui(wargs->pipe_ui_fd,
                                                           incoming_req->seq,
                                                           res);
                                }
                                if (incoming_req->stop_req.stop_name)
                                        free(incoming_req->stop_req.stop_name);
                                if (incoming_req->stop_req.route_name)
                                        free(incoming_req->stop_req.route_name);
                        } else if (incoming_req->type ==
                                   LOGIC_TASK_ITINERARY_REQ) {
                                struct itinerary_res *res = get_itinerary(
                                        &incoming_req->itinerary_req,
                                        wdata->raptor_stops,
                                        wdata->n_stops_built,
                                        wdata->n_routes_built,
                                        wdata->raptor_stop_map);
                                if (res != NULL) {
                                        send_itinerary_res_ui(wargs->pipe_ui_fd,
                                                              incoming_req->seq,
                                                              res);
                                }
                        } else if (incoming_req->type ==
                                   LOGIC_TASK_SHAPES_REQ) {
                                struct shape_res *res = get_flat_shapes(
                                        wdata->trips, wdata->n_trips,
                                        wdata->n_shapes, wdata->shape_map,
                                        wdata->n_stops, wdata->lat_ref,
                                        incoming_req->shape_req);
                                send_shape_res_ui(wargs->pipe_ui_fd,
                                                  incoming_req->seq, res);
                        } else if (incoming_req->type ==
                                   LOGIC_TASK_ITINERARY_SHAPES_REQ) {
                                struct shape_res *res = get_itinerary_flat_shapes(
                                        incoming_req->itinerary_shape_req
                                                .itinerary,
                                        wdata->n_shapes, wdata->shape_map,
                                        wdata->n_stops, wdata->lat_ref,
                                        incoming_req->itinerary_shape_req);
                                send_shape_res_ui(wargs->pipe_ui_fd,
                                                  incoming_req->seq, res);
                        }
                        free(incoming_req);
                }
        }
        free(wargs);
        return NULL;
}
