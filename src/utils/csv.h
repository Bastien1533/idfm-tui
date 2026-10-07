#ifndef CSV_H
#define CSV_H

#include <stdlib.h>

typedef struct {
        char **headers;
        size_t col_count;

        char ***rows;
        size_t row_count;
} csv_file;

csv_file *read_csv_file(const char *filename, const char delimiter);
void free_csv(csv_file *csv);

#endif
