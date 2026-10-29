#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <notcurses/notcurses.h>

#include "algo/raptor.h"
#include "data/gtfs.h"
#include "tui/ui/selector.h"
#include "tui/scenes/scenes.h"
#include "utils/utils.h"

void add_itinerary_item_selector(ui_selector *selector, char type, char *id,
                                 const char *fmt, ...)
{
        va_list arglist;
        va_start(arglist, fmt);
        char *desc;
        char *opt;

        if (vasprintf(&desc, fmt, arglist) != -1) {
                struct ncselector_item *item =
                        calloc(1, sizeof(struct ncselector_item));
                if (item == NULL) {
                        va_end(arglist);
                        return;
                }
                item->desc = desc;
                int stat = (id == NULL) ? asprintf(&opt, "%c", type) :
                                          asprintf(&opt, "%c|%s", type, id);
                if (stat == -1) {
                        item->option = NULL;
                        free_selector_item(item);
                } else {
                        item->option = opt;
                        stat = uiselector_add_item(selector, item, NULL);
                        if (stat == 1)
                                free_selector_item(item);
                }
        }
        va_end(arglist);
}

void itinerary_format_raptor_selector(struct raptor_itinerary *itinerary,
                                      ui_selector *sel)
{
        if (itinerary == NULL) {
                return;
        }
        for (size_t j = itinerary->n_legs; j > 0; j--) {
                struct raptor_leg *leg = itinerary->legs[j - 1];
                if (leg == NULL || leg->from == NULL || leg->to == NULL ||
                    leg->from->stop == NULL || leg->to->stop == NULL) {
                        continue;
                }
                if (leg->trip != NULL) {
                        char *dep_stop_hhmmss = print_hhmmss(leg->dep_time);
                        char *arr_stop_hhmmss = print_hhmmss(leg->arr_time);
                        add_itinerary_item_selector(sel, 'S',
                                                    leg->from->stop->stop_id,
                                                    "%s || %s (%s)",
                                                    dep_stop_hhmmss,
                                                    leg->from->stop->stop_name,
                                                    leg->from->stop->stop_id);
                        add_itinerary_item_selector(
                                sel, 'R', leg->trip->route->route_id,
                                "         || %s - %s ",
                                leg->trip->route->route_short_name,
                                leg->trip->trip_headsign);
                        add_itinerary_item_selector(sel, '\0', NULL,
                                                    "         ||");
                        add_itinerary_item_selector(sel, 'S',
                                                    leg->to->stop->stop_id,
                                                    "%s || %s (%s)",
                                                    arr_stop_hhmmss,
                                                    leg->to->stop->stop_name,
                                                    leg->to->stop->stop_id);
                        free(dep_stop_hhmmss);
                        free(arr_stop_hhmmss);
                }
                if (leg->transfer != NULL && j != itinerary->n_legs && j != 1) {
                        add_itinerary_item_selector(
                                sel, '\0', NULL, "Transfer | %hu min \n",
                                (leg->transfer->min_transfer_time / 60));
                }
        }
}