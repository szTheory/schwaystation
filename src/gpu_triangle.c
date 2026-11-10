/* Integer flat-polygon coverage follows docs/hardware/gpu.md#polygons. */
#include "gpu.h"

#include "internal.h"

struct schway__gpu_point {
    int32_t x;
    int32_t y;
};

struct schway__gpu_edge {
    int64_t value;
    int64_t step_x;
    int64_t step_y;
};

static int32_t sign11(uint32_t value)
{
    value &= 0x7FFu;
    return (value & 0x400u) != 0u ? (int32_t)value - 0x800 : (int32_t)value;
}

static int64_t edge_value(const struct schway__gpu_point *a,
                          const struct schway__gpu_point *b, int32_t x, int32_t y)
{
    int64_t dx = (int64_t)b->x - a->x;
    int64_t dy = (int64_t)b->y - a->y;
    int64_t pixel_x = (int64_t)x * 2 + 1 - (int64_t)a->x * 2;
    int64_t pixel_y = (int64_t)y * 2 + 1 - (int64_t)a->y * 2;
    return dx * pixel_y - dy * pixel_x;
}

static int64_t triangle_area(const struct schway__gpu_point *a,
                             const struct schway__gpu_point *b,
                             const struct schway__gpu_point *c)
{
    int64_t ab_x = (int64_t)b->x - a->x;
    int64_t ab_y = (int64_t)b->y - a->y;
    int64_t ac_x = (int64_t)c->x - a->x;
    int64_t ac_y = (int64_t)c->y - a->y;
    return ab_x * ac_y - ab_y * ac_x;
}

static struct schway__gpu_edge make_edge(const struct schway__gpu_point *a,
                                         const struct schway__gpu_point *b,
                                         int32_t x, int32_t y)
{
    int64_t dx = (int64_t)b->x - a->x;
    int64_t dy = (int64_t)b->y - a->y;
    uint32_t top_left = dy < 0 || (dy == 0 && dx > 0);
    struct schway__gpu_edge edge;
    edge.value = edge_value(a, b, x, y) - (top_left != 0u ? 0 : 1);
    edge.step_x = -2 * dy;
    edge.step_y = 2 * dx;
    return edge;
}

static uint32_t spans_allowed(const struct schway__gpu_point *points, uint32_t count)
{
    int32_t min_x = points[0].x;
    int32_t max_x = points[0].x;
    int32_t min_y = points[0].y;
    int32_t max_y = points[0].y;
    for (uint32_t i = 1u; i < count; i++) {
        if (points[i].x < min_x) min_x = points[i].x;
        if (points[i].x > max_x) max_x = points[i].x;
        if (points[i].y < min_y) min_y = points[i].y;
        if (points[i].y > max_y) max_y = points[i].y;
    }
    return (int64_t)max_x - min_x <= 1023 && (int64_t)max_y - min_y <= 511;
}

static void render_triangle(schway *inst, struct schway__gpu_point a,
                            struct schway__gpu_point b, struct schway__gpu_point c,
                            uint32_t color, uint32_t flags)
{
    const struct schway__gpu_point triangle_points[3] = {a, b, c};
    if (spans_allowed(triangle_points, 3u) == 0u) {
        return;
    }
    int64_t area = triangle_area(&a, &b, &c);
    if (area == 0) {
        return;
    }
    if (area < 0) {
        struct schway__gpu_point swap = b;
        b = c;
        c = swap;
    }

    int32_t min_x = a.x < b.x ? a.x : b.x;
    int32_t max_x = a.x > b.x ? a.x : b.x;
    int32_t min_y = a.y < b.y ? a.y : b.y;
    int32_t max_y = a.y > b.y ? a.y : b.y;
    if (c.x < min_x) min_x = c.x;
    if (c.x > max_x) max_x = c.x;
    if (c.y < min_y) min_y = c.y;
    if (c.y > max_y) max_y = c.y;

    const struct schway__gpu *g = &inst->gpu;
    int32_t left = (int32_t)(g->draw_area_tl & 0x3FFu);
    int32_t top = (int32_t)((g->draw_area_tl >> 10) & 0x1FFu);
    int32_t right = (int32_t)(g->draw_area_br & 0x3FFu);
    int32_t bottom = (int32_t)((g->draw_area_br >> 10) & 0x1FFu);
    if (left > right || top > bottom) {
        return;
    }

    if (min_x < left) min_x = left;
    if (min_y < top) min_y = top;
    if (max_x > right + 1) max_x = right + 1;
    if (max_y > bottom + 1) max_y = bottom + 1;
    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x > (int32_t)SCHWAY__VRAM_W) max_x = (int32_t)SCHWAY__VRAM_W;
    if (max_y > (int32_t)SCHWAY__VRAM_H) max_y = (int32_t)SCHWAY__VRAM_H;
    if (min_x >= max_x || min_y >= max_y) {
        return;
    }

    struct schway__gpu_edge ab = make_edge(&a, &b, min_x, min_y);
    struct schway__gpu_edge bc = make_edge(&b, &c, min_x, min_y);
    struct schway__gpu_edge ca = make_edge(&c, &a, min_x, min_y);
    int64_t row_ab = ab.value;
    int64_t row_bc = bc.value;
    int64_t row_ca = ca.value;
    for (int32_t y = min_y; y < max_y; y++) {
        int64_t value_ab = row_ab;
        int64_t value_bc = row_bc;
        int64_t value_ca = row_ca;
        for (int32_t x = min_x; x < max_x; x++) {
            if (value_ab >= 0 && value_bc >= 0 && value_ca >= 0) {
                schway__gpu_pixel(inst, (uint32_t)x, (uint32_t)y, 0u, color, flags);
            }
            value_ab += ab.step_x;
            value_bc += bc.step_x;
            value_ca += ca.step_x;
        }
        row_ab += ab.step_y;
        row_bc += bc.step_y;
        row_ca += ca.step_y;
    }
}

void schway__gpu_triangle(schway *inst, const int32_t vertices[3][2], uint32_t color,
                          uint32_t flags)
{
    if (inst == NULL || vertices == NULL) {
        return;
    }
    struct schway__gpu_point a = {vertices[0][0], vertices[0][1]};
    struct schway__gpu_point b = {vertices[1][0], vertices[1][1]};
    struct schway__gpu_point c = {vertices[2][0], vertices[2][1]};
    render_triangle(inst, a, b, c, color, flags);
}

void schway__gpu_polygon(schway *inst, const uint32_t *position_words, uint32_t vertex_count,
                         uint32_t color, uint32_t flags)
{
    if (inst == NULL || position_words == NULL || (vertex_count != 3u && vertex_count != 4u)) {
        return;
    }
    int32_t offset_x = sign11(inst->gpu.draw_offset);
    int32_t offset_y = sign11(inst->gpu.draw_offset >> 11);
    struct schway__gpu_point points[4];
    for (uint32_t i = 0u; i < vertex_count; i++) {
        points[i].x = sign11(position_words[i]) + offset_x;
        points[i].y = sign11(position_words[i] >> 16) + offset_y;
    }
    if (spans_allowed(points, vertex_count) == 0u) {
        return;
    }

    struct schway__gpu_point first[3] = {points[0], points[1], points[2]};
    render_triangle(inst, first[0], first[1], first[2], color, flags);
    if (vertex_count == 4u) {
        struct schway__gpu_point second[3] = {points[1], points[2], points[3]};
        render_triangle(inst, second[0], second[1], second[2], color, flags);
    }
}
