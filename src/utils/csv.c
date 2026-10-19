#include "utils/csv.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

char **parse_csv_line(char *line, size_t *out_cols, const char delimiter,
                      long col_count_i)
{
        line[strcspn(line, "\r\n")] = '\0';
        char delim_str[2] = { delimiter, '\0' };

        size_t col_count = 0;
        if (col_count_i == -1) {
                char *temp = line;
                while (*temp)
                        if (*temp++ == delimiter)
                                ++col_count;
                col_count++;
        } else {
                col_count = col_count_i;
        }

        char **parsed_line = malloc(sizeof(char *) * col_count);

        unsigned char *p = (unsigned char *)line;
        if (p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF)
                line += 3; // skip UTF-8 BOM

        char *curr = line;
        char *field_start_c = NULL;
        size_t field_len = 0;
        bool in_quotes = false;

        size_t i = 0;
        while (*curr != '\0') {
                char c = *curr;
                if (c == '\r' || c == '\n')
                        break;
                if (c == '"') {
                        if (in_quotes && *(curr + 1) == '"') {
                                field_len++;
                                curr++;
                        } else
                                in_quotes = !in_quotes;
                } else if (c == ',' && !in_quotes) {
                        char *tok = malloc(field_len + 1);
                        memcpy(tok, field_start_c, field_len);
                        tok[field_len] = '\0';
                        parsed_line[i] = tok;
                        field_start_c = NULL;
                        field_len = 0;
                        i++;
                } else {
                        if (field_start_c == NULL)
                                field_start_c = curr;
                        field_len++;
                }
                curr++;
        }
        char *tok = malloc(field_len + 1);
        memcpy(tok, field_start_c, field_len);
        tok[field_len] = '\0';
        parsed_line[i] = tok;
        field_start_c = NULL;
        field_len = 0;

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
                                                        delimiter, -1);
                        } else {
                                rows[line_idx - 1] = parse_csv_line(
                                        buf, NULL, delimiter, col_count);
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
