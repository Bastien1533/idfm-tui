#include "utils/csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **parse_csv_line(char *line, size_t *out_cols, const char delimiter)
{
        line[strcspn(line, "\r\n")] = '\0';
        char delim_str[2] = { delimiter, '\0' };

        size_t col_count = 0;
        char *temp = line;
        while (*temp)
                if (*temp++ == delimiter)
                        ++col_count;
        col_count++; // One more column that there is of ;

        char **parsed_line = malloc(sizeof(char *) * col_count);

        char *curr = line;
        char *tok;
        size_t i = 0;
        while ((tok = strsep(&curr, delim_str)) != NULL && i < col_count) {
                parsed_line[i] = strdup(tok);
                i++;
        }
        if (out_cols != NULL) {
                *out_cols = col_count;
        }
        return parsed_line;
}

csv_file *read_csv_file(const char *filename, const char delimiter)
{
        if (filename != NULL) {
                FILE *fp = fopen(filename, "r");
                if (!fp) {
                        return NULL;
                }

                csv_file *csv = malloc(
                        sizeof(csv_file)); // Malloc for the csv_file struct
                if (!csv) {
                        return NULL;
                }

                size_t col_count = 0;
                size_t row_count = 0;

                size_t ch;
                while (!feof(fp)) {
                        ch = fgetc(fp);
                        if (ch == '\n') {
                                row_count++;
                        }
                }
                rewind(fp);
                row_count--; // Count header

                char **header;
                char ***rows = malloc(row_count * sizeof(char **));

                char buf[4096];
                size_t line_idx = 0;
                while (fgets(buf, sizeof(buf), fp) != NULL) {
                        if (line_idx == 0) {
                                header = parse_csv_line(buf, &col_count,
                                                        delimiter);
                        } else {
                                rows[line_idx - 1] = parse_csv_line(
                                        buf, &col_count, delimiter);
                        }
                        line_idx++;
                }
                fclose(fp);

                csv->headers = header;
                csv->rows = rows;
                csv->row_count = row_count;
                csv->col_count = col_count;
                return csv;
        }
        return NULL;
}

void free_csv(csv_file *csv)
{
        if (csv == NULL) {
                return;
        }
        if (csv->headers != NULL) {
                for (size_t i = 0; i < csv->col_count; i++) {
                        free(csv->headers[i]);
                }
                free(csv->headers);
        }

        if (csv->rows != NULL) {
                for (size_t r = 0; r < csv->row_count; r++) {
                        if (csv->rows[r] != NULL) {
                                for (size_t c = 0; c < csv->col_count; c++) {
                                        free(csv->rows[r][c]);
                                }
                                free(csv->rows[r]);
                        }
                }
                free(csv->rows);
        }
        free(csv);
}
