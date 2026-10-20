/* The internal state walk. One field list drives six passes over the same
   bytes: SIZE counts them, WRITE stores the fields, VALIDATE reads the bytes
   and range-checks every field without touching the instance, APPLY reads
   them into the fields, RECORD emits each field's name, offset, width, count
   and bounds for the tests' rejection sweep, and VISIT hands each field to
   the public visitor. VALIDATE and APPLY walk the same list over the same
   buffer, so once VALIDATE passes, APPLY cannot fail (ARCHITECTURE
   section 5).

   Every multi-byte value is written one byte at a time, least significant
   first, so the bytes are the same on every platform. Not installed. */
#ifndef SCHWAYSTATION_STATE_H
#define SCHWAYSTATION_STATE_H

#include <stddef.h>
#include <stdint.h>

#include "internal.h"

/* One field as RECORD reports it: offset from the start of the state, width
   of one value in bytes, number of values, and the inclusive range VALIDATE
   accepts for each. */
struct schway__state_field {
    const char *name;
    uint32_t offset;
    uint32_t width;
    uint32_t count;
    uint64_t min, max;
};

/* Writes up to cap field records to out and returns how many fields the
   state has (which may exceed cap); 0 for NULL. */
size_t schway__state_record(const schway *inst, struct schway__state_field *out, size_t cap);

/* Recomputes the body checksum of a state buffer of size bytes, so a test
   can change one field and still reach the range checks. */
void schway__state_seal(void *buf, size_t size);

enum {
    SCHWAY__STATE_SIZE,
    SCHWAY__STATE_WRITE,
    SCHWAY__STATE_VALIDATE,
    SCHWAY__STATE_APPLY,
    SCHWAY__STATE_RECORD,
    SCHWAY__STATE_VISIT
};

struct schway__state_io {
    int mode;
    uint8_t *w;                       /* WRITE cursor */
    const uint8_t *r;                 /* VALIDATE and APPLY cursor */
    size_t n;                         /* SIZE and RECORD: bytes counted so far */
    int bad;                          /* VALIDATE: a range or cross-field rule failed */
    struct schway__state_field *rec;  /* RECORD: output, or NULL to only count */
    size_t rec_cap;                   /* RECORD: capacity of rec */
    size_t rec_n;                     /* RECORD: fields seen, which may exceed rec_cap */
    schway_state_visitor visitor;     /* VISIT */
    void *user;                       /* VISIT */
};

static inline void schway__state_record_one(struct schway__state_io *io, const char *name,
                                            uint32_t width, uint32_t count, uint64_t min,
                                            uint64_t max)
{
    if (io->rec != NULL && io->rec_n < io->rec_cap) {
        struct schway__state_field *f = &io->rec[io->rec_n];
        f->name = name;
        f->offset = (uint32_t)io->n;
        f->width = width;
        f->count = count;
        f->min = min;
        f->max = max;
    }
    io->rec_n++;
}

/* Moves one value of width bytes, little-endian, bounded to [min, max].
   Returns the value: v for SIZE, WRITE, RECORD and VISIT, the decoded value
   for VALIDATE and APPLY. */
static inline uint64_t schway__state_field_io(struct schway__state_io *io, const char *name,
                                              uint64_t v, uint32_t width, uint64_t min,
                                              uint64_t max)
{
    switch (io->mode) {
    case SCHWAY__STATE_RECORD:
        schway__state_record_one(io, name, width, 1u, min, max);
        io->n += width;
        return v;
    case SCHWAY__STATE_SIZE:
        io->n += width;
        return v;
    case SCHWAY__STATE_VISIT:
        return v;
    case SCHWAY__STATE_WRITE:
        for (uint32_t i = 0; i < width; i++) {
            io->w[i] = (uint8_t)(v >> (8u * i));
        }
        io->w += width;
        return v;
    default:
        v = 0u;
        for (uint32_t i = 0; i < width; i++) {
            v |= (uint64_t)io->r[i] << (8u * i);
        }
        io->r += width;
        if (io->mode == SCHWAY__STATE_VALIDATE && (v < min || v > max)) {
            io->bad = 1;
        }
        return v;
    }
}

/* A cross-field rule: in VALIDATE, a false ok marks the state invalid. */
static inline void schway__state_check(struct schway__state_io *io, int ok)
{
    if (io->mode == SCHWAY__STATE_VALIDATE && !ok) {
        io->bad = 1;
    }
}

static inline uint32_t schway__state_u32(struct schway__state_io *io, const char *name,
                                         uint32_t *v, uint32_t min, uint32_t max)
{
    if (io->mode == SCHWAY__STATE_VISIT) {
        io->visitor(io->user, name, v, 1u, 4u);
        return *v;
    }
    uint32_t x = (uint32_t)schway__state_field_io(io, name, *v, 4u, min, max);
    if (io->mode == SCHWAY__STATE_APPLY) {
        *v = x;
    }
    return x;
}

static inline uint64_t schway__state_u64(struct schway__state_io *io, const char *name,
                                         uint64_t *v, uint64_t min, uint64_t max)
{
    if (io->mode == SCHWAY__STATE_VISIT) {
        io->visitor(io->user, name, v, 1u, 8u);
        return *v;
    }
    uint64_t x = schway__state_field_io(io, name, *v, 8u, min, max);
    if (io->mode == SCHWAY__STATE_APPLY) {
        *v = x;
    }
    return x;
}

/* An array of count halfwords, one field: RECORD and VISIT see it once. */
static inline void schway__state_u16_array(struct schway__state_io *io, const char *name,
                                           uint16_t *a, uint32_t count, uint16_t min,
                                           uint16_t max)
{
    switch (io->mode) {
    case SCHWAY__STATE_RECORD:
        schway__state_record_one(io, name, 2u, count, min, max);
        io->n += (size_t)count * 2u;
        return;
    case SCHWAY__STATE_SIZE:
        io->n += (size_t)count * 2u;
        return;
    case SCHWAY__STATE_VISIT:
        io->visitor(io->user, name, a, count, 2u);
        return;
    case SCHWAY__STATE_WRITE:
        for (uint32_t i = 0; i < count; i++) {
            io->w[2u * i] = (uint8_t)(a[i] & 0xFFu);
            io->w[2u * i + 1u] = (uint8_t)(a[i] >> 8);
        }
        io->w += (size_t)count * 2u;
        return;
    default:
        for (uint32_t i = 0; i < count; i++) {
            uint16_t x = (uint16_t)(io->r[2u * i] | (io->r[2u * i + 1u] << 8));
            if (io->mode == SCHWAY__STATE_APPLY) {
                a[i] = x;
            } else if (x < min || x > max) {
                io->bad = 1;
            }
        }
        io->r += (size_t)count * 2u;
        return;
    }
}

#endif /* SCHWAYSTATION_STATE_H */
