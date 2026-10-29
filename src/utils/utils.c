#include "data/dataset.h"
#include "data/rules.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum route_mode string_to_mode(const char *string)
{
        if (string == NULL)
                return UNKNOWN;

        if (strcmp(string, "Bus") == 0)
                return BUS;
        if (strcmp(string, "Metro") == 0)
                return METRO;
        if (strcmp(string, "Tramway") == 0)
                return TRAMWAY;
        if (strcmp(string, "CableWay") == 0)
                return CABLE;
        if (strcmp(string, "LocalTrain") == 0)
                return COMMUTER_RAIL;
        if (strcmp(string, "RapidTransit") == 0)
                return COMMUTER_RAIL;
        if (strcmp(string, "regionalRail") == 0)
                return REGIONAL_RAIL;
        if (strcmp(string, "RailShuttle") == 0)
                return AIRPORT_RAIL;
        if (strcmp(string, "Funicular") == 0)
                return FUNICULAR;

        return UNKNOWN; // Fallback for unmatched/empty strings
}

bool line_in_array(struct dataset_line **lines, char *stop_id, size_t n)
{
        for (size_t i = 0; i < n; i++) {
                if (strcmp(lines[i]->route_id, stop_id) == 0) {
                        return true;
                }
        }
        return false;
}

bool string_eq(char *a, char *b)
{
        return (strcmp(a, b) == 0) == true;
}

char *print_hhmmss(unsigned long sec)
{
        unsigned long h = (sec / 3600) % 24;
        unsigned long m = (sec % 3600) / 60;
        unsigned long s = sec % 60;
        char *res = NULL;
        asprintf(&res, "%02lu:%02lu:%02lu", h, m, s);
        return res;
}

char *print_hhmm(unsigned long sec)
{
        unsigned long h = (sec / 3600) % 24;
        unsigned long m = (sec % 3600) / 60;
        char *res = NULL;
        asprintf(&res, "%02lu:%02lu", h, m);
        return res;
}

void clear_array(void *array, size_t len, size_t elem_size)
{
        memset(array, 0, len * elem_size);
}

unsigned long min_int(unsigned long a, unsigned long b)
{
        return a < b ? a : b;
}

int csv_col_index(csv_file *csv, const char *name)
{
        for (size_t i = 0; i < csv->col_count; i++) {
                char *header = csv->headers[i];
                if (strncmp(header, "\xEF\xBB\xBF", 3) == 0) {
                        header += 3;
                }
                if (strcmp(header, name) == 0) {
                        return (int)i;
                }
        }
        return -1;
}

unsigned long int gtfs_time_from_midnight(const char *s)
{
        if (!s || s[0] == '\0' || s[2] != ':' || s[5] != ':')
                return 0;

        unsigned long h = (s[0] - '0') * 10 + (s[1] - '0');
        unsigned long m = (s[3] - '0') * 10 + (s[4] - '0');
        unsigned long sec = (s[6] - '0') * 10 + (s[7] - '0');
        return h * 3600 + m * 60 + sec;
}
