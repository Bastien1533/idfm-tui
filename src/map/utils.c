#include "map/geo.h"
#include "map/shapes.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include "map/map.h"
#include "map/geo.h"
#include <stdlib.h>
#include <string.h>

bool put_cell(struct cell_pos pt, struct map_view *map_view,
              struct map_cell *cells, uint32_t fg, uint32_t bg, uint32_t ch,
              double z_index)
{
        if (pt.col < 0 || pt.row < 0 || pt.col >= (int)map_view->cols ||
            pt.row >= (int)map_view->rows)
                return false;
        if (cells[(pt.row * map_view->cols) + pt.col].z_index > z_index)
                return false;
        struct map_cell c = { .ch = ch == 0 ? 'o' : ch,
                              .fg = fg,
                              .bg = bg,
                              .min_scale = -1,
                              .max_scale = -1,
                              .z_index = z_index };
        cells[(pt.row * map_view->cols) + pt.col] = c;
        return true;
}

size_t paint_bresenham_high(struct cell_pos pt1, struct cell_pos pt2,
                            struct map_view *map_view, struct map_cell *cells,
                            uint32_t color, double z_index)
{
        size_t n_painted = 0;
        long x0 = pt1.col;
        long y0 = pt1.row;
        long x1 = pt2.col;
        long y1 = pt2.row;

        long dx = x1 - x0;
        long dy = y1 - y0;
        long xi = 1;
        if (dx < 0) {
                xi = -1;
                dx = -dx;
        }
        long d = (2 * dx) - dy;
        long x = x0;
        for (long y = y0; y <= y1; y++) {
                struct cell_pos pt = { .col = x, .row = y };
                put_cell(pt, map_view, cells, color, 0, 0, z_index);
                n_painted++;
                if (d > 0) {
                        x += xi;
                        d += (2 * (dx - dy));
                } else {
                        d += 2 * dx;
                }
        }
        return n_painted;
}

size_t paint_bresenham_low(struct cell_pos pt1, struct cell_pos pt2,
                           struct map_view *map_view, struct map_cell *cells,
                           uint32_t color, double z_index)
{
        size_t n_painted = 0;
        long x0 = pt1.col;
        long y0 = pt1.row;
        long x1 = pt2.col;
        long y1 = pt2.row;

        long dx = x1 - x0;
        long dy = y1 - y0;
        long yi = 1;
        if (dy < 0) {
                yi = -1;
                dy = -dy;
        }
        long d = (2 * dy) - dx;
        long y = y0;
        for (long x = x0; x <= x1; x++) {
                struct cell_pos pt = { .col = x, .row = y };
                put_cell(pt, map_view, cells, color, 0, 0, z_index);
                n_painted++;
                if (d > 0) {
                        y += yi;
                        d += (2 * (dy - dx));
                } else {
                        d += 2 * dy;
                }
        }
        return n_painted;
}

bool paint_bresenham_line(struct cell_pos pt1, struct cell_pos pt2,
                          struct map_view *map_view, struct map_cell *cells,
                          size_t *n_painted, uint32_t color, double z_index)
{
        long x0 = pt1.col;
        long y0 = pt1.row;
        long x1 = pt2.col;
        long y1 = pt2.row;
        if (labs(y1 - y0) < labs(x1 - x0)) {
                if (x0 > x1)
                        (*n_painted) += paint_bresenham_low(
                                pt2, pt1, map_view, cells, color, z_index);
                else
                        (*n_painted) += paint_bresenham_low(
                                pt1, pt2, map_view, cells, color, z_index);
        } else {
                if (y0 > y1)
                        (*n_painted) += paint_bresenham_high(
                                pt2, pt1, map_view, cells, color, z_index);
                else
                        (*n_painted) += paint_bresenham_high(
                                pt1, pt2, map_view, cells, color, z_index);
        }
        return true;
}

bool paint_horiz_line(struct flat_pt start, struct flat_pt end,
                      size_t orientation, struct map_view *map_view,
                      struct map_cell *cells, size_t *n_painted)
{
        struct cell_pos source = project_point(start, map_view);
        struct cell_pos source_end = project_point(end, map_view);
        size_t len = source_end.col - source.col;

        for (size_t col = source.col; col < source.col + len; col++) {
                struct map_cell c = { .ch = 'o' };
                cells[(source.row * map_view->cols) + col] = c;
                (*n_painted)++;
        }
        return true;
}

bool paint_flat_line(struct flat_pt *points, size_t n_points,
                     struct map_view *map_view, struct map_cell *cells,
                     size_t *n_painted, uint32_t color, double z_index)
{
        if (points == NULL || n_points <= 1)
                return false;
        for (size_t i = 0; i < n_points - 1; i++) {
                struct bbox seg_box = get_bbox(points[i], points[i + 1]);
                if (check_bbox_intersect(seg_box, map_view->view_box) == false)
                        continue;
                struct flat_pt na;
                struct flat_pt nb;
                if (compute_liang_barksy_line(map_view->view_box, points[i],
                                              points[i + 1], &na, &nb) == false)
                        continue;

                struct cell_pos pt1 = project_point(na, map_view);
                struct cell_pos pt2 = project_point(nb, map_view);
                paint_bresenham_line(pt1, pt2, map_view, cells, n_painted,
                                     color, z_index);
        }
        return true;
}

bool paint_circle(double mx, double my, int radius, struct map_view *map_view,
                  struct map_cell *cells, size_t *n_painted, uint32_t color,
                  double z_index)
{
        struct cell_pos c =
                project_point((struct flat_pt){ .x = mx, .y = my }, map_view);

        if (radius < 1) {
                if (put_cell(c, map_view, cells, color, 0, 0, z_index))
                        (*n_painted)++;
                return true;
        }

        int hy = (int)lround(radius *
                             map_view->aspect); /* vertical half-extent */
        if (hy < 1)
                hy = 1;

        for (int dy = -hy; dy <= hy; dy++) {
                double t = 1.0 - ((double)dy * dy) / ((double)hy * hy);
                if (t < 0.0)
                        continue;
                int dx = (int)lround(radius * sqrt(t));
                for (int x = -dx; x <= dx; x++) {
                        struct cell_pos pt = { .col = c.col + x,
                                               .row = c.row + dy };
                        if (put_cell(pt, map_view, cells, color, 0, 0, z_index))
                                (*n_painted)++;
                }
        }
        return true;
}

bool paint_label(double mx, double my, char *label, struct map_view *map_view,
                 struct map_cell *cells, size_t *n_painted, uint32_t color,
                 double z_index)
{
        struct cell_pos c =
                project_point((struct flat_pt){ .x = mx, .y = my }, map_view);
        size_t len = strlen(label);
        c.col += 5;
        for (size_t i = 0; i < len;) {
                uint32_t egc = 0;
                unsigned char b = (unsigned char)label[i];
                size_t char_len = (b < 0x80)           ? 1 :
                                  ((b & 0xE0) == 0xC0) ? 2 :
                                  ((b & 0xF0) == 0xE0) ? 3 :
                                                         4;
                if (i + char_len > len)
                        break;
                memcpy(&egc, &label[i], char_len);
                i += char_len;
                c.col++;
                if (put_cell(c, map_view, cells, color, 0, egc, z_index))
                        (*n_painted)++;
        }
        return true;
}

static bool shape_visible(const struct flat_shape *s, const struct map_view *v)
{
        if (v->scale / 200 <= s->min_scale) /* too far out */
                return false;
        if (s->max_scale > 0 && v->scale / 200 >= s->max_scale) /* too far in */
                return false;
        return true;
}

bool shape_valid(const struct flat_shape *s)
{
        if (s->type == LINE && s->as.line.n_pts < 2)
                return false;
        if (s->type == RECTANGLE && (s->as.rect.xmax == s->as.rect.ymin ||
                                     s->as.rect.xmax == s->as.rect.ymin))
                return false;
        return true;
}

bool paint_flat_shape(struct flat_shape shape, struct map_view *map_view,
                      struct map_cell *cells, size_t *n_painted)
{
        if (!shape_visible(&shape, map_view))
                return false;
        if (shape.type == LINE)
                return paint_flat_line(shape.as.line.pts, shape.as.line.n_pts,
                                       map_view, cells, n_painted, shape.color,
                                       shape.z_index);
        if (shape.type == CIRCLE)
                return paint_circle(shape.as.circle.mx, shape.as.circle.my,
                                    shape.as.circle.radius, map_view, cells,
                                    n_painted, shape.color, shape.z_index);
        if (shape.type == TEXT)
                return paint_label(shape.as.text.mx, shape.as.text.my,
                                   shape.as.text.text, map_view, cells,
                                   n_painted, shape.color, shape.z_index);
        return false;
}
