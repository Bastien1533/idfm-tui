#include "utils/utils.h"

#include <notcurses/notcurses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "algo/raptor.h"
#include "data/gtfs.h"
#include "tui/scenes/itinerary/itinerary.h"
#include "tui/scenes/scenes.h"
#include "tui/ui/input.h"
#include "tui/ui/selector.h"

void add_itinerary_item_selector(ui_selector *selector, char type, char *id,
                                 const char *fmt, ...)
{
    va_list arglist;
    va_start(arglist, fmt);
    char *desc;
    char *opt;

    if (vasprintf(&desc, fmt, arglist) != -1)
    {
        struct ncselector_item *item =
            calloc(1, sizeof(struct ncselector_item));
        if (item == NULL)
        {
            va_end(arglist);
            return;
        }
        item->desc = desc;
        int stat = (id == NULL) ? asprintf(&opt, "%c", type)
                                : asprintf(&opt, "%c|%s", type, id);
        if (stat == -1)
        {
            item->option = NULL;
            free_selector_item(item);
        }
        else
        {
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
    if (itinerary == NULL)
    {
        return;
    }
    for (size_t j = itinerary->n_legs; j > 0; j--)
    {
        struct raptor_leg *leg = itinerary->legs[j - 1];
        if (leg == NULL || leg->from == NULL || leg->to == NULL
            || leg->from->stop == NULL || leg->to->stop == NULL)
        {
            continue;
        }
        if (leg->trip != NULL)
        {
            char *dep_stop_hhmmss = print_hhmmss(leg->dep_time);
            char *arr_stop_hhmmss = print_hhmmss(leg->arr_time);
            add_itinerary_item_selector(sel, 'S', leg->from->stop->stop_id,
                                        "%s || %s", dep_stop_hhmmss,
                                        leg->from->stop->stop_name);
            add_itinerary_item_selector(
                sel, 'R', leg->trip->route->route_id, "         || %s - %s ",
                leg->trip->route->route_short_name, leg->trip->trip_headsign);
            add_itinerary_item_selector(sel, '\0', NULL, "         ||");
            add_itinerary_item_selector(sel, 'S', leg->to->stop->stop_id,
                                        "%s || %s", arr_stop_hhmmss,
                                        leg->to->stop->stop_name);
            free(dep_stop_hhmmss);
            free(arr_stop_hhmmss);
        }
        if (leg->transfer != NULL && j != itinerary->n_legs && j != 1)
        {
            add_itinerary_item_selector(
                sel, '\0', NULL, "Transfer | %hu min \n",
                (leg->transfer->min_transfer_time / 60));
        }
    }
}

bool is_valid_time_str(char *str, size_t *len, bool *has_sep)
{
    size_t len_t = strlen(str);

    bool has_sep_t = false;
    for (size_t i = 0; i < len_t; i++)
    {
        bool is_num = str[i] <= '9' && str[i] >= '0';
        bool is_sep = str[i] == ':';
        if ((!is_num && !is_sep) || (is_sep && has_sep_t))
            return false;

        if (is_sep)
            has_sep_t = true;
    }
    if (!has_sep_t && len_t > 4)
        return false;
    *has_sep = has_sep_t;
    *len = len_t;
    return true;
}

bool parse_time(char *str, size_t len, bool has_sep, unsigned long *time)
{
    unsigned int h = 0;
    unsigned int m = 0;

    if (has_sep)
    {
        sscanf(str, "%u:%u", &h, &m);
    }
    else
    {
        if (len == 1)
            h = str[0] - '0';
        else if (len == 2)
            h = (str[0] - '0') * 10 + str[1] - '0';
        else if (len == 3)
        {
            h = str[0] - '0';
            m = (str[1] - '0') * 10 + str[2] - '0';
        }
        else if (len == 4)
        {
            h = (str[0] - '0') * 10 + str[1] - '0';
            m = (str[2] - '0') * 10 + str[3] - '0';
        }
        else
            return false;
    }
    if (h > 23 || m > 59)
        return false;
    *time = h * 3600 + m * 60;
    return true;
}

void itinerary_set_dep_time(ui_input *ipt, void *payload)
{
    if (payload == NULL || ipt == NULL || ipt->reader == NULL)
        return;
    scene_t *scene = payload;
    itinerary_ctx *i_ctx = scene->payload;
    char *str = ncreader_contents(ipt->reader);

    size_t len;
    bool has_sep;

    bool is_valid = is_valid_time_str(str, &len, &has_sep);
    if (is_valid == false)
    {
        char *time_str = print_hhmm(i_ctx->sel_dep_time);
        set_input_text(ipt, time_str);
        free(time_str);
        return;
    }

    unsigned long time;
    bool stat = parse_time(str, len, has_sep, &time);
    if (stat == false)
        time = i_ctx->sel_dep_time;

    char *time_str = print_hhmm(time);
    set_input_text(ipt, time_str);
    free(time_str);
    if (stat == true)
        i_ctx->sel_dep_time = time;
}
