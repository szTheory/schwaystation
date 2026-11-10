/* Integer polygon coverage and attribute interpolation follow docs/hardware/gpu.md#polygons. */
#include <limits.h>

#include "gpu.h"

#include "internal.h"

#define SCHWAY__GPU_ATTR_FRACTION_BITS 16u
#define SCHWAY__GPU_ATTR_ONE (INT64_C(1) << SCHWAY__GPU_ATTR_FRACTION_BITS)
#define SCHWAY__GPU_ATTR_COUNT 5u

struct schway__gpu_point {
    int32_t x;
    int32_t y;
};

struct schway__gpu_vertex {
    struct schway__gpu_point point;
    uint32_t color;
    uint32_t uv;
};

struct schway__gpu_edge {
    int64_t value;
    int64_t step_x;
    int64_t step_y;
};

struct schway__gpu_plane {
    int64_t area;
    int64_t dx_numerator[SCHWAY__GPU_ATTR_COUNT];
    int64_t dy_numerator[SCHWAY__GPU_ATTR_COUNT];
    int32_t step_x[SCHWAY__GPU_ATTR_COUNT];
    int32_t step_y[SCHWAY__GPU_ATTR_COUNT];
    uint32_t steppers_valid;
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

static int64_t divide_rounded(int64_t numerator, int64_t denominator)
{
    if (numerator < 0) {
        return -((-numerator + denominator / 2) / denominator);
    }
    return (numerator + denominator / 2) / denominator;
}

static int32_t vertex_attribute(const struct schway__gpu_vertex *vertex, uint32_t attribute)
{
    if (attribute < 3u) {
        return (int32_t)((vertex->color >> (attribute * 8u)) & 0xFFu);
    }
    return (int32_t)((vertex->uv >> ((attribute - 3u) * 8u)) & 0xFFu);
}

static int32_t store_stepper(int64_t step, uint32_t *valid)
{
    if (step > INT32_MAX) {
        *valid = 0u;
        return INT32_MAX;
    }
    if (step < INT32_MIN) {
        *valid = 0u;
        return INT32_MIN;
    }
    return (int32_t)step;
}

static void setup_plane(const struct schway__gpu_vertex *a,
                        const struct schway__gpu_vertex *b,
                        const struct schway__gpu_vertex *c, int64_t area,
                        struct schway__gpu_plane *plane)
{
    int64_t ab_x = (int64_t)b->point.x - a->point.x;
    int64_t ab_y = (int64_t)b->point.y - a->point.y;
    int64_t ac_x = (int64_t)c->point.x - a->point.x;
    int64_t ac_y = (int64_t)c->point.y - a->point.y;
    plane->area = area;
    plane->steppers_valid = 1u;
    for (uint32_t i = 0u; i < SCHWAY__GPU_ATTR_COUNT; i++) {
        int64_t delta_b = (int64_t)vertex_attribute(b, i) - vertex_attribute(a, i);
        int64_t delta_c = (int64_t)vertex_attribute(c, i) - vertex_attribute(a, i);
        int64_t dx = delta_b * ac_y - delta_c * ab_y;
        int64_t dy = ab_x * delta_c - ac_x * delta_b;
        plane->dx_numerator[i] = dx;
        plane->dy_numerator[i] = dy;
        plane->step_x[i] = store_stepper(divide_rounded(dx * SCHWAY__GPU_ATTR_ONE, area),
                                         &plane->steppers_valid);
        plane->step_y[i] = store_stepper(divide_rounded(dy * SCHWAY__GPU_ATTR_ONE, area),
                                         &plane->steppers_valid);
    }
}

static int64_t attribute_at_sample(const struct schway__gpu_vertex *a,
                                   const struct schway__gpu_plane *plane,
                                   uint32_t attribute, int32_t x, int32_t y)
{
    int64_t sample_x = (int64_t)x * 2 + 1 - (int64_t)a->point.x * 2;
    int64_t sample_y = (int64_t)y * 2 + 1 - (int64_t)a->point.y * 2;
    int64_t denominator = plane->area * 2;
    int64_t numerator = (int64_t)vertex_attribute(a, attribute) * denominator +
                        plane->dx_numerator[attribute] * sample_x +
                        plane->dy_numerator[attribute] * sample_y;
    return divide_rounded(numerator * SCHWAY__GPU_ATTR_ONE, denominator);
}

static uint32_t attribute_channel(int64_t value)
{
    if (value <= 0) {
        return 0u;
    }
    if (value >= INT64_C(255) * SCHWAY__GPU_ATTR_ONE) {
        return 255u;
    }
    return (uint32_t)(value / SCHWAY__GPU_ATTR_ONE);
}

static uint32_t pixel_color(const int64_t values[SCHWAY__GPU_ATTR_COUNT])
{
    uint32_t red = attribute_channel(values[0]);
    uint32_t green = attribute_channel(values[1]);
    uint32_t blue = attribute_channel(values[2]);
    return red | (green << 8) | (blue << 16);
}

static uint32_t pixel_uv(const int64_t values[SCHWAY__GPU_ATTR_COUNT], uint32_t clut)
{
    uint32_t u = attribute_channel(values[3]);
    uint32_t v = attribute_channel(values[4]);
    return (u & 0xFFu) | ((v & 0xFFu) << 8) | clut;
}

static void render_triangle(schway *inst, struct schway__gpu_vertex a,
                            struct schway__gpu_vertex b, struct schway__gpu_vertex c,
                            uint32_t flags, uint32_t clut)
{
    const struct schway__gpu_point input_points[3] = {a.point, b.point, c.point};
    if (spans_allowed(input_points, 3u) == 0u) {
        return;
    }
    int64_t area = triangle_area(&a.point, &b.point, &c.point);
    if (area == 0) {
        return;
    }
    if (area < 0) {
        struct schway__gpu_vertex swap = b;
        b = c;
        c = swap;
        area = -area;
    }

    int32_t min_x = a.point.x < b.point.x ? a.point.x : b.point.x;
    int32_t max_x = a.point.x > b.point.x ? a.point.x : b.point.x;
    int32_t min_y = a.point.y < b.point.y ? a.point.y : b.point.y;
    int32_t max_y = a.point.y > b.point.y ? a.point.y : b.point.y;
    if (c.point.x < min_x) min_x = c.point.x;
    if (c.point.x > max_x) max_x = c.point.x;
    if (c.point.y < min_y) min_y = c.point.y;
    if (c.point.y > max_y) max_y = c.point.y;

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

    struct schway__gpu_plane plane;
    setup_plane(&a, &b, &c, area, &plane);
    struct schway__gpu_edge ab = make_edge(&a.point, &b.point, min_x, min_y);
    struct schway__gpu_edge bc = make_edge(&b.point, &c.point, min_x, min_y);
    struct schway__gpu_edge ca = make_edge(&c.point, &a.point, min_x, min_y);
    int64_t row_ab = ab.value;
    int64_t row_bc = bc.value;
    int64_t row_ca = ca.value;
    int64_t row_values[SCHWAY__GPU_ATTR_COUNT];
    for (uint32_t i = 0u; i < SCHWAY__GPU_ATTR_COUNT; i++) {
        row_values[i] = attribute_at_sample(&a, &plane, i, min_x, min_y);
    }

    for (int32_t y = min_y; y < max_y; y++) {
        int64_t value_ab = row_ab;
        int64_t value_bc = row_bc;
        int64_t value_ca = row_ca;
        int64_t values[SCHWAY__GPU_ATTR_COUNT];
        for (uint32_t i = 0u; i < SCHWAY__GPU_ATTR_COUNT; i++) {
            values[i] = row_values[i];
        }
        for (int32_t x = min_x; x < max_x; x++) {
            if (value_ab >= 0 && value_bc >= 0 && value_ca >= 0) {
                if (plane.steppers_valid == 0u) {
                    for (uint32_t i = 0u; i < SCHWAY__GPU_ATTR_COUNT; i++) {
                        values[i] = attribute_at_sample(&a, &plane, i, x, y);
                    }
                }
                uint32_t color = pixel_color(values);
                uint16_t texel = 0u;
                if ((flags & SCHWAY__GPU_PIXEL_TEXTURED) != 0u) {
                    texel = schway__gpu_texel(inst, pixel_uv(values, clut));
                }
                schway__gpu_pixel(inst, (uint32_t)x, (uint32_t)y, texel, color, flags);
            }
            value_ab += ab.step_x;
            value_bc += bc.step_x;
            value_ca += ca.step_x;
            if (plane.steppers_valid != 0u) {
                for (uint32_t i = 0u; i < SCHWAY__GPU_ATTR_COUNT; i++) {
                    values[i] += plane.step_x[i];
                }
            }
        }
        row_ab += ab.step_y;
        row_bc += bc.step_y;
        row_ca += ca.step_y;
        if (plane.steppers_valid != 0u) {
            for (uint32_t i = 0u; i < SCHWAY__GPU_ATTR_COUNT; i++) {
                row_values[i] += plane.step_y[i];
            }
        } else {
            for (uint32_t i = 0u; i < SCHWAY__GPU_ATTR_COUNT; i++) {
                row_values[i] = attribute_at_sample(&a, &plane, i, min_x, y + 1);
            }
        }
    }
}

void schway__gpu_triangle(schway *inst, const int32_t vertices[3][2], uint32_t color,
                          uint32_t flags)
{
    if (inst == NULL || vertices == NULL) {
        return;
    }
    struct schway__gpu_vertex a = {{vertices[0][0], vertices[0][1]}, color, 0u};
    struct schway__gpu_vertex b = {{vertices[1][0], vertices[1][1]}, color, 0u};
    struct schway__gpu_vertex c = {{vertices[2][0], vertices[2][1]}, color, 0u};
    render_triangle(inst, a, b, c, flags, 0u);
}

void schway__gpu_polygon(schway *inst, const uint32_t *position_words,
                         const uint32_t *color_words, const uint32_t *uv_words,
                         uint32_t vertex_count, uint32_t flags)
{
    if (inst == NULL || position_words == NULL || color_words == NULL ||
        (vertex_count != 3u && vertex_count != 4u)) {
        return;
    }
    if ((flags & SCHWAY__GPU_PIXEL_TEXTURED) != 0u && uv_words == NULL) {
        return;
    }
    int32_t offset_x = sign11(inst->gpu.draw_offset);
    int32_t offset_y = sign11(inst->gpu.draw_offset >> 11);
    struct schway__gpu_vertex vertices[4];
    struct schway__gpu_point points[4];
    for (uint32_t i = 0u; i < vertex_count; i++) {
        vertices[i].point.x = sign11(position_words[i]) + offset_x;
        vertices[i].point.y = sign11(position_words[i] >> 16) + offset_y;
        vertices[i].color = color_words[i] & 0x00FFFFFFu;
        vertices[i].uv = uv_words != NULL ? uv_words[i] & 0x0000FFFFu : 0u;
        points[i] = vertices[i].point;
    }
    uint32_t clut = 0u;
    if ((flags & SCHWAY__GPU_PIXEL_TEXTURED) != 0u) {
        clut = uv_words[0] & 0xFFFF0000u;
        uint32_t page = (uv_words[1] >> 16) & 0x1FFu;
        inst->gpu.draw_mode = (inst->gpu.draw_mode & ~0x1FFu) | page;
    }
    if (spans_allowed(points, vertex_count) == 0u) {
        return;
    }
    render_triangle(inst, vertices[0], vertices[1], vertices[2], flags, clut);
    if (vertex_count == 4u) {
        render_triangle(inst, vertices[1], vertices[2], vertices[3], flags, clut);
    }
}
