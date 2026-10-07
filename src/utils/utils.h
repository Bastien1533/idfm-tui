#ifndef UTILS_H
#define UTILS_H
#include "utils/csv.h"
#include "data/dataset.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool line_in_array(struct dataset_line **lines, char *stop_id, size_t n);
bool string_eq(char *a, char *b);
unsigned long int gtfs_time_from_midnight(const char *time_string);
int csv_col_index(csv_file *csv, const char *name);
char *print_hhmmss(unsigned long sec);
void clear_array(void *array, size_t len, size_t elem_size);
unsigned long min_int(unsigned long a, unsigned long b);
#endif
