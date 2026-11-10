/* A seeded integer generator for property tests: the same seed gives the same
   stream on every platform, so a failure prints its seed and replays.

   Why it is not a library: the property-testing libraries either fork a
   process for every case, which not every lane can do (the wasm32 and
   big-endian lanes run the tests under an emulator or a runtime that has no
   fork), or bring in a dependency. A 60-line generator is the smaller tool. It
   is splitmix64, which has no state beyond one 64-bit word and passes the
   usual statistical batteries; it is not for anything secret. */
#ifndef SCHWAYSTATION_TESTS_PROP_H
#define SCHWAYSTATION_TESTS_PROP_H

#include <stddef.h>
#include <stdint.h>

struct prop {
    uint64_t s;
};

static inline void prop_seed(struct prop *p, uint64_t seed)
{
    p->s = seed;
}

/* The next 64 bits of the stream (splitmix64). */
static inline uint64_t prop_next(struct prop *p)
{
    uint64_t z;
    p->s += UINT64_C(0x9E3779B97F4A7C15);
    z = p->s;
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/* A value in 0..n-1 (n of 1 to 2^32), from the high bits, which are the
   better ones. It is biased by less than 2^-32, which no test here can see. */
static inline uint32_t prop_below(struct prop *p, uint32_t n)
{
    return (uint32_t)(((prop_next(p) >> 32) * (uint64_t)n) >> 32);
}

/* An entry of the edges table half the time, and a random 32-bit word the
   other half: the values where hardware rules change are found by aiming at
   them, not by luck. */
static inline uint32_t prop_edge32(struct prop *p, const uint32_t *edges, size_t n)
{
    if (n != 0u && prop_below(p, 2u) == 0u) {
        return edges[prop_below(p, (uint32_t)n)];
    }
    return (uint32_t)(prop_next(p) >> 32);
}

#endif /* SCHWAYSTATION_TESTS_PROP_H */
