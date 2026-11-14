#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "algo/raptor.h"
#include "data/gtfs.h"
#include "tui/protocol.h"
#include "tui/ui/selector.h"
#include "utils/hashmap.h"
#include "utils/utils.h"

char **raptor_to_strings(struct raptor_result *res, size_t *n_res)
{
    if (res == NULL || res->dep == NULL || res->arr == NULL
        || res->itineraries == NULL)
    {
        *n_res = 0;
        return NULL;
    }

    char **res_str = calloc(res->n_itineraries, sizeof(char *));

    for (size_t i = 0; i < res->n_itineraries; i++)
    {
        struct raptor_itinerary *itinerary = res->itineraries[i];
        if (itinerary == NULL)
        {
            // printf("Itineray %zu is not valid\n", i);
            continue;
        }
        /*printf("------------- Itinerary %zu | %zu trips -------------\n",
               i, itinerary->n_trips);*/
        char *str = strdup("");
        for (size_t j = itinerary->n_legs; j > 0; j--)
        {
            struct raptor_leg *leg = itinerary->legs[j - 1];
            if (leg == NULL || leg->from == NULL || leg->to == NULL
                || leg->from->stop == NULL || leg->to->stop == NULL)
            {
                // printf("leg %zu is not valid\n", j - 1);
                continue;
            }
            if (leg->trip != NULL)
            {
                if (j == 2)
                {
                    char *dst = NULL;
                    asprintf(&dst, "%s%s || %s || %s", str,
                             leg->from->stop->stop_name,
                             leg->trip->route->route_short_name,
                             leg->to->stop->stop_name);
                    free(str);
                    str = dst;
                }

                else
                {
                    char *dst = NULL;
                    asprintf(&dst, "%s%s || %s -> ", str,
                             leg->from->stop->stop_name,
                             leg->trip->route->route_short_name);
                    free(str);
                    str = dst;
                }
            }
            /*if (leg->transfer != NULL) {
                    asprintf(&str, "%sTransfer | %hu min \n", str,
                             (leg->transfer->min_transfer_time /
                              60));
            } */
        }
        char *hhmmss_dep = print_hhmmss(itinerary->dep_time);
        char *hhmmss_arr = print_hhmmss(itinerary->arr_time);
        char *dst = NULL;
        asprintf(&dst, "%s (%s -> %s)", str, hhmmss_dep, hhmmss_arr);
        free(str);
        free(hhmmss_dep);
        free(hhmmss_arr);
        str = dst;
        res_str[i] = str;
    }
    *n_res = res->n_itineraries;
    return res_str;
}

struct itinerary_res *get_itinerary(logic_task_itinerary_req *request,
                                    struct raptor_stop **raptor_stops,
                                    size_t n_raptor_stops,
                                    size_t n_raptor_routes,
                                    struct hash_map *raptor_stop_map)
{
    struct raptor_stop *arr = hash_map_get(raptor_stop_map, request->arr_id);
    struct raptor_stop *dep = hash_map_get(raptor_stop_map, request->dep_id);
    struct raptor_result *res = NULL;
    if (dep && arr)
    {
        /*printf("%s -> %s\n", dep->stop->stop_name,
               arr->stop->stop_name); */
        res = raptor_algo(n_raptor_routes, raptor_stops, n_raptor_stops,
                          request->dep_time, dep, arr);
        if (res == NULL)
        {
            return NULL;
        }
        size_t n_res;
        char **res_strings = raptor_to_strings(res, &n_res);

        struct itinerary_res *i_res = calloc(1, sizeof(struct itinerary_res));
        struct ncselector_item **items =
            calloc(n_res, sizeof(struct ncselector_item *));
        if (i_res == NULL || items == NULL)
        {
            if (i_res == NULL)
                free(items);
            if (items == NULL)
                free(i_res);
            free(res_strings);
            free_raptor_result(res, true);
            return NULL;
        }

        for (size_t i = 0; i < n_res; i++)
        {
            struct ncselector_item *item =
                calloc(1, sizeof(struct ncselector_item));
            if (item != NULL)
            {
                item->desc = res_strings[i];
                char *option;
                int stat = asprintf(&option, "%zu", i);
                if (stat == -1)
                {
                    item->option = NULL;
                    free_selector_item(item);
                    item = NULL;
                    items[i] = item;
                    break;
                }
                item->option = option;
                items[i] = item;
            }
        }
        free(res_strings);

        i_res->search_items = items;
        i_res->itineraries = res->itineraries;
        i_res->n_itineraries = res->n_itineraries;
        free_raptor_result(res, false);
        free(res);
        return i_res;
    }
    return NULL;
}
