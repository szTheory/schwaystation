/* An independent model of the geometry engine, for the differential test.
   It is written from the public hardware description (psx-spx, Geometry
   Transformation Engine) and the hardware-captured cases alone: plain
   integer fields, one function per command, no packed words and nothing
   shared with the engine under src/. The three functions mirror the engine's
   register and command entry points, so tests/gte/vectors.h replays the same
   cases through either. */
#ifndef SCHWAYSTATION_TESTS_GTE_REF_H
#define SCHWAYSTATION_TESTS_GTE_REF_H

#include <stdint.h>

/* One plain field per architectural quantity. Where hardware has an array of
   like registers the field is an array, and index 0 of mac and ir is MAC0 and
   IR0. Values are held as the numbers they mean, not as register patterns: a
   signed 16-bit register is its signed value, an unsigned one its unsigned
   value, so no field needs a sign fix-up on use. */
struct gte_ref {
    int32_t vx[3], vy[3], vz[3];                  /* V0 to V2 */
    int32_t rgbc_r, rgbc_g, rgbc_b, rgbc_c;       /* RGBC bytes and CODE */
    int32_t otz;                                  /* 0 to FFFFh */
    int32_t ir[4];                                /* IR0 to IR3 */
    int32_t sx[3], sy[3];                         /* the screen XY FIFO, oldest first */
    int32_t sz[4];                                /* the screen Z FIFO, oldest first */
    int32_t rgb_r[3], rgb_g[3], rgb_b[3], rgb_c[3]; /* the colour FIFO, oldest first */
    int32_t res1;                                 /* the prohibited register 23, a full word */
    int32_t mac[4];                               /* MAC0 to MAC3 */
    int32_t lzcs;
    int32_t rt[3][3], ll[3][3], lc[3][3];         /* rotation, light and colour matrices */
    int32_t tr[3], bk[3], fc[3];                  /* translation, background and far colour */
    int32_t ofx, ofy;
    int32_t h;                                    /* 0 to FFFFh */
    int32_t dqa, dqb;
    int32_t zsf3, zsf4;
    int32_t flag;                                 /* bits 30 to 12 as software wrote or the engine set them */
};

/* A register as software writes it (MTC2, CTC2): idx 0 to 31 are the data
   registers and 32 to 63 the control registers. */
void gte_ref_write(struct gte_ref *r, uint32_t idx, uint32_t v);

/* A register as software reads it (MFC2, CFC2). */
uint32_t gte_ref_read(const struct gte_ref *r, uint32_t idx);

/* Runs the command in the low 25 bits of op and returns the cycles it takes. A
   command number the hardware description does not list changes nothing and
   takes 0. */
uint32_t gte_ref_exec(struct gte_ref *r, uint32_t op);

/* The two test cases that use the model (tests/gte/ref_tests.c); each returns
   the process exit status. */
int ref_run_vectors(int argc, char **argv);
int ref_run_property(int argc, char **argv);

#endif /* SCHWAYSTATION_TESTS_GTE_REF_H */
