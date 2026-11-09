/* Replays the SingleStepTests r3000 vectors through schway__cpu_step_one:
     schwaystation-test-vectors FILE --expect N [--only NAME,...] [--allow FILE]
                                [--flip-expected FIELD]
   FILE holds the committed sample: a u16 slice count, then per slice a 16-byte
   NUL-padded name, a u32 byte length and the upstream bytes (a u32 case count,
   then the cases), all little-endian. A case is a 51-byte Pascal name, a u32
   opcode and address, an initial and a final state of 172 bytes (32 GPRs, HI,
   LO, EPC, TAR, CAUSE, PC, then branch target, slot and take, load target
   and value), a u32 cycle count and 24 bytes per cycle (val, actions, addr and
   size, the first and third as 64-bit values). The bus is replayed from the
   cycles; the final state is compared field by field. Exit 0 when every case
   passes and N or more ran, 1 for a failure or a malformed file, 2 for a bad
   command line. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"

#define NAME_BYTES 51u
#define STATE_BYTES 172u
#define CYCLE_BYTES 24u
#define SLICE_NAME_BYTES 16u
#define MAX_NAME 50u
#define MAX_CYCLES 8u
#define MAX_CASES 1000u
#define MAX_LISTED 64u
#define PRINT_CASES 12u
#define NO_LOAD_TARGET 0xFFFFFFFFu

struct vstate {
    uint32_t r[32];
    uint32_t hi, lo, epc, tar, cause, pc;
    uint32_t branch_target, branch_slot, branch_take;
    uint32_t load_target, load_value; /* the target is -1 when none */
};

struct vcase {
    char name[NAME_BYTES + 1u];
    uint32_t opcode, addr;
    struct vstate initial, final;
    uint32_t ncycles;
    struct schway__cpu_cycle cycle[MAX_CYCLES];
};

struct slice {
    char name[SLICE_NAME_BYTES + 1u];
    const uint8_t *data;
    uint32_t len, cases;
    int selected;
};

struct allowed {
    char file[SLICE_NAME_BYTES + 1u];
    char hex[16];
    int used;
};

static const char *path;
static uint8_t *buf;
static struct slice *slices;
static schway *inst;
static struct allowed allow[MAX_LISTED];
static uint32_t allow_n;

static void finish(int code)
{
    schway_destroy(inst);
    free(slices);
    free(buf);
    exit(code);
}

static void die(const char *what)
{
    fprintf(stderr, "vectors: %s: %s\n", path, what);
    finish(1);
}

static void usage(void)
{
    fprintf(stderr, "usage: schwaystation-test-vectors FILE --expect N [--only NAME,...] "
                    "[--allow FILE] [--flip-expected FIELD]\n");
    finish(2);
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

/* The whole file, with a NUL after the last byte. */
static uint8_t *read_file(const char *name, size_t *len)
{
    FILE *f = fopen(name, "rb");
    if (f == NULL) {
        return NULL;
    }
    uint8_t *b = NULL;
    size_t n = 0u;
    if (fseek(f, 0, SEEK_END) == 0) {
        long end = ftell(f);
        if (end >= 0 && fseek(f, 0, SEEK_SET) == 0) {
            n = (size_t)end;
            b = malloc(n + 1u);
        }
    }
    if (b != NULL && fread(b, 1u, n, f) != n) {
        free(b);
        b = NULL;
    }
    (void)fclose(f);
    if (b != NULL) {
        b[n] = 0u;
        *len = n;
    }
    return b;
}

/* One state record at p, 172 bytes. */
static void parse_state(const uint8_t *p, struct vstate *s)
{
    for (uint32_t i = 0; i < 32u; i++) {
        s->r[i] = rd32(p + 4u * i);
    }
    s->hi = rd32(p + 128);
    s->lo = rd32(p + 132);
    s->epc = rd32(p + 136);
    s->tar = rd32(p + 140);
    s->cause = rd32(p + 144);
    s->pc = rd32(p + 148);
    s->branch_target = rd32(p + 152);
    s->branch_slot = rd32(p + 156);
    s->branch_take = rd32(p + 160);
    s->load_target = rd32(p + 164);
    s->load_value = rd32(p + 168);
    if (s->r[0] != 0u) {
        die("a state has a non-zero R0");
    }
    if (s->branch_slot > 1u || s->branch_take > 1u) {
        die("a branch flag is neither 0 nor 1");
    }
    if (s->load_target > 31u && s->load_target != NO_LOAD_TARGET) {
        die("a load target is neither -1 nor a register");
    }
}

/* One case at *pp, which must end inside [*pp, end). Every malformed field is
   refused: the file is data, and a quiet clamp would hide a bad one. */
static void parse_case(const uint8_t **pp, const uint8_t *end, struct vcase *c)
{
    const uint8_t *p = *pp;
    if ((size_t)(end - p) < NAME_BYTES + 8u + 2u * STATE_BYTES + 4u) {
        die("truncated case");
    }
    uint32_t len = p[0];
    if (len > MAX_NAME) {
        die("a case name is longer than 50 bytes");
    }
    for (uint32_t i = 0; i < len; i++) {
        uint8_t ch = p[1u + i];
        c->name[i] = (ch >= 0x20u && ch < 0x7Fu) ? (char)ch : '?';
    }
    c->name[len] = '\0';
    p += NAME_BYTES;
    c->opcode = rd32(p);
    c->addr = rd32(p + 4);
    p += 8;
    parse_state(p, &c->initial);
    p += STATE_BYTES;
    parse_state(p, &c->final);
    p += STATE_BYTES;
    c->ncycles = rd32(p);
    p += 4;
    if (c->ncycles > MAX_CYCLES) {
        die("a cycle count is over 8");
    }
    if ((size_t)(end - p) < (size_t)c->ncycles * CYCLE_BYTES) {
        die("truncated case cycles");
    }
    for (uint32_t i = 0; i < c->ncycles; i++) {
        if (rd32(p + 4) != 0u || rd32(p + 16) != 0u) {
            die("a cycle value or address has a non-zero high dword");
        }
        c->cycle[i].value = rd32(p);
        c->cycle[i].kind = rd32(p + 8);
        c->cycle[i].addr = rd32(p + 12);
        c->cycle[i].size = rd32(p + 20);
        p += CYCLE_BYTES;
    }
    *pp = p;
}

/* Reads the container and every case in it, so a malformed file is refused
   whatever --only selects. Returns the slice count. */
static uint32_t parse_container(size_t n)
{
    if (n == 0u) {
        die("empty file");
    }
    if (n < 2u) {
        die("short file");
    }
    uint32_t count = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8);
    if (count == 0u) {
        die("no slices");
    }
    slices = calloc(count, sizeof slices[0]);
    if (slices == NULL) {
        die("out of memory");
    }
    size_t off = 2u;
    for (uint32_t i = 0; i < count; i++) {
        if (n - off < SLICE_NAME_BYTES + 4u) {
            die("short file: a slice header is cut");
        }
        struct slice *s = &slices[i];
        memcpy(s->name, buf + off, SLICE_NAME_BYTES);
        s->name[SLICE_NAME_BYTES] = '\0';
        s->len = rd32(buf + off + SLICE_NAME_BYTES);
        off += SLICE_NAME_BYTES + 4u;
        if (n - off < s->len) {
            die("truncated slice");
        }
        if (s->len < 4u) {
            die("short slice");
        }
        s->data = buf + off;
        s->cases = rd32(s->data);
        if (s->cases > MAX_CASES) {
            die("a case count is over 1000");
        }
        const uint8_t *p = s->data + 4;
        const uint8_t *end = s->data + s->len;
        struct vcase scratch;
        for (uint32_t k = 0; k < s->cases; k++) {
            parse_case(&p, end, &scratch);
        }
        if (p != end) {
            die("a slice is longer than its cases");
        }
        off += s->len;
    }
    if (off != n) {
        die("long file: bytes after the last slice");
    }
    return count;
}

/* The next space-separated word of *p, cut off with a NUL, or NULL at the end
   of the line. */
static char *next_word(char **p)
{
    char *w = *p;
    while (*w == ' ' || *w == '\t' || *w == '\r') {
        w++;
    }
    if (*w == '\0') {
        return NULL;
    }
    char *e = w;
    while (*e != '\0' && *e != ' ' && *e != '\t' && *e != '\r') {
        e++;
    }
    if (*e != '\0') {
        *e++ = '\0';
    }
    *p = e;
    return w;
}

/* One allow line: a date, a file name, a case and an https URL, nothing else. */
static int parse_allow_line(char *line, struct allowed *out)
{
    char *date = next_word(&line);
    char *file = next_word(&line);
    char *hex = next_word(&line);
    char *url = next_word(&line);
    if (url == NULL || next_word(&line) != NULL || strlen(date) != 10u ||
        strlen(file) > SLICE_NAME_BYTES || strlen(hex) >= sizeof out->hex ||
        strncmp(url, "https://", 8u) != 0) {
        return 0;
    }
    for (uint32_t i = 0; i < 10u; i++) {
        int digit = date[i] >= '0' && date[i] <= '9';
        if (i == 4u || i == 7u ? date[i] != '-' : !digit) {
            return 0;
        }
    }
    memcpy(out->file, file, strlen(file) + 1u);
    memcpy(out->hex, hex, strlen(hex) + 1u);
    out->used = 0;
    return 1;
}

/* The dated lines of allow.txt: YYYY-MM-DD <file> <case-hex> <issue URL>. */
static void load_allow(const char *name)
{
    size_t n = 0u;
    uint8_t *text = read_file(name, &n);
    if (text == NULL) {
        fprintf(stderr, "vectors: cannot read %s\n", name);
        finish(1);
    }
    char *line = (char *)text;
    while (*line != '\0') {
        char *eol = line;
        while (*eol != '\0' && *eol != '\n') {
            eol++;
        }
        int last = *eol == '\0';
        *eol = '\0';
        if (line[0] != '#' && line[0] != '\0') {
            if (allow_n == MAX_LISTED || !parse_allow_line(line, &allow[allow_n])) {
                fprintf(stderr, "vectors: %s: bad line\n", name);
                free(text);
                finish(1);
            }
            allow_n++;
        }
        line = last ? eol : eol + 1;
    }
    free(text);
}

/* The allow line of a failing case, or NULL. A case's hex is the text after
   the $ in its name. */
static struct allowed *find_allow(const struct slice *s, const struct vcase *c)
{
    const char *hex = strchr(c->name, '$');
    for (uint32_t i = 0; hex != NULL && i < allow_n; i++) {
        if (strcmp(allow[i].file, s->name) == 0 && strcmp(allow[i].hex, hex + 1) == 0) {
            return &allow[i];
        }
    }
    return NULL;
}

struct run {
    const struct slice *slice;
    const struct vcase *c;
    uint32_t mismatches;
    int print;
};

/* A mismatch is counted for every case and printed only for the first few
   failing ones, so a broken executor does not bury the first line. */
static void cycle_mismatch(struct run *r, uint32_t k, const char *field, uint32_t want,
                           uint32_t got)
{
    r->mismatches++;
    if (r->print) {
        fprintf(stderr, "%s %s cycle %u %s: want 0x%08x got 0x%08x\n", r->slice->name,
                r->c->name, (unsigned)k, field, (unsigned)want, (unsigned)got);
    }
}

static void check(struct run *r, const char *field, uint32_t want, uint32_t got)
{
    if (want == got) {
        return;
    }
    r->mismatches++;
    if (r->print) {
        fprintf(stderr, "%s %s final %s: want 0x%08x got 0x%08x\n", r->slice->name,
                r->c->name, field, (unsigned)want, (unsigned)got);
    }
}

/* Loads the initial state into a freshly reset CPU. SR is 0: with the
   interrupt and mode bits clear nothing but the instruction decides the
   result. */
static void load_state(struct schway__cpu *cpu, const struct vstate *s)
{
    memcpy(cpu->r, s->r, sizeof cpu->r);
    cpu->hi = s->hi;
    cpu->lo = s->lo;
    cpu->pc = s->pc;
    cpu->cop0.r[SCHWAY__COP0_EPC] = s->epc;
    cpu->cop0.r[SCHWAY__COP0_TAR] = s->tar;
    cpu->cop0.r[SCHWAY__COP0_CAUSE] = s->cause;
    cpu->cop0.r[SCHWAY__COP0_SR] = 0u;
    cpu->branch_target = s->branch_target;
    cpu->branch_slot = s->branch_slot;
    cpu->branch_take = s->branch_take;
    cpu->load_reg = s->load_target == NO_LOAD_TARGET ? SCHWAY__CPU_NO_LOAD : s->load_target;
    cpu->load_value = s->load_value;
}

static void compare_cycles(struct run *r, const struct schway__cpu_log *log)
{
    const struct vcase *c = r->c;
    uint32_t total = log->got_n + log->overflow;
    for (uint32_t k = 0; k < log->got_n && k < c->ncycles; k++) {
        const struct schway__cpu_cycle *want = &c->cycle[k];
        const struct schway__cpu_cycle *got = &log->got[k];
        if (got->kind != want->kind) {
            cycle_mismatch(r, k, "kind", want->kind, got->kind);
        }
        if (got->size != want->size) {
            cycle_mismatch(r, k, "size", want->size, got->size);
        }
        if (got->addr != want->addr) {
            cycle_mismatch(r, k, "addr", want->addr, got->addr);
        }
        if (got->value != want->value) {
            cycle_mismatch(r, k, "value", want->value, got->value);
        }
    }
    if (total != c->ncycles) {
        cycle_mismatch(r, total < c->ncycles ? total : c->ncycles, "count", c->ncycles, total);
    }
}

static void compare_final(struct run *r, const struct schway__cpu *cpu)
{
    const struct vstate *w = &r->c->final;
    char field[8];
    for (uint32_t i = 0; i < 32u; i++) {
        (void)snprintf(field, sizeof field, "r%u", (unsigned)i);
        check(r, field, w->r[i], cpu->r[i]);
    }
    check(r, "hi", w->hi, cpu->hi);
    check(r, "lo", w->lo, cpu->lo);
    check(r, "pc", w->pc, cpu->pc);
    check(r, "epc", w->epc, cpu->cop0.r[SCHWAY__COP0_EPC]);
    check(r, "tar", w->tar, cpu->cop0.r[SCHWAY__COP0_TAR]);
    check(r, "cause", w->cause, cpu->cop0.r[SCHWAY__COP0_CAUSE]);
    check(r, "branch_target", w->branch_target, cpu->branch_target);
    check(r, "branch_slot", w->branch_slot, cpu->branch_slot);
    check(r, "branch_take", w->branch_take, cpu->branch_take);
    check(r, "load_target", w->load_target,
          cpu->load_reg == SCHWAY__CPU_NO_LOAD ? NO_LOAD_TARGET : cpu->load_reg);
    check(r, "load_value", w->load_value, cpu->load_value);
}

/* Inverts one bit of the first case's expected final FIELD, so a negative
   control can show that the field is compared. Returns 0 for a name that is
   not one of the six. */
static int flip(struct vstate *f, const char *field)
{
    if (strcmp(field, "r5") == 0) {
        f->r[5] ^= 1u;
    } else if (strcmp(field, "epc") == 0) {
        f->epc ^= 1u;
    } else if (strcmp(field, "tar") == 0) {
        f->tar ^= 1u;
    } else if (strcmp(field, "cause") == 0) {
        f->cause ^= 1u;
    } else if (strcmp(field, "branch_target") == 0) {
        f->branch_target ^= 1u;
    } else if (strcmp(field, "load_value") == 0) {
        f->load_value ^= 1u;
    } else {
        return 0;
    }
    return 1;
}

static void *vec_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void vec_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static void make_instance(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = vec_alloc;
    cfg.allocator.free = vec_free;
    if (schway_create(&cfg, &inst) != SCHWAY_STATUS_OK) {
        die("cannot create an instance");
    }
}

/* Replays the cases of one selected slice, adding to *ran every case run,
   to *failures the failing ones and to *absorbed those an allow line covers. */
static void run_slice(struct slice *s, const char *flip_field, unsigned long *ran,
                      unsigned long *failures, unsigned long *absorbed, unsigned *printed)
{
    const uint8_t *p = s->data + 4;
    const uint8_t *end = s->data + s->len;
    for (uint32_t k = 0; k < s->cases; k++) {
        struct vcase c;
        struct schway__cpu_log log;
        parse_case(&p, end, &c);
        if (*ran == 0u && flip_field != NULL) {
            (void)flip(&c.final, flip_field);
        }
        struct allowed *a = find_allow(s, &c);
        struct run r = {s, &c, 0u, a == NULL && *printed < PRINT_CASES};

        schway__cpu_reset(inst);
        load_state(&inst->cpu, &c.initial);
        memset(&log, 0, sizeof log);
        log.expect = c.cycle;
        log.expect_n = c.ncycles;
        schway__cpu_step_one(inst, &log);
        compare_cycles(&r, &log);
        compare_final(&r, &inst->cpu);

        (*ran)++;
        if (r.mismatches != 0u && a != NULL) {
            a->used = 1;
            (*absorbed)++;
        } else if (r.mismatches != 0u) {
            (*failures)++;
            (*printed)++;
        }
    }
}

static unsigned long parse_count(const char *text)
{
    char *rest = NULL;
    unsigned long v = strtoul(text, &rest, 10);
    if (text[0] < '0' || text[0] > '9' || *rest != '\0' || v == 0u) {
        usage();
    }
    return v;
}

int main(int argc, char **argv)
{
    const char *only = NULL, *allow_path = NULL, *flip_field = NULL;
    unsigned long expect = 0u;
    path = NULL;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int has_value = i + 1 < argc;
        if (strcmp(a, "--expect") == 0 && has_value) {
            expect = parse_count(argv[++i]);
        } else if (strcmp(a, "--only") == 0 && has_value) {
            only = argv[++i];
        } else if (strcmp(a, "--allow") == 0 && has_value) {
            allow_path = argv[++i];
        } else if (strcmp(a, "--flip-expected") == 0 && has_value) {
            flip_field = argv[++i];
        } else if (a[0] != '-' && path == NULL) {
            path = a;
        } else {
            usage();
        }
    }
    struct vstate probe;
    memset(&probe, 0, sizeof probe);
    if (path == NULL || expect == 0u || (flip_field != NULL && !flip(&probe, flip_field))) {
        usage();
    }

    size_t n = 0u;
    buf = read_file(path, &n);
    if (buf == NULL) {
        die("cannot read the file");
    }
    uint32_t count = parse_container(n);
    if (allow_path != NULL) {
        load_allow(allow_path);
    }

    /* --only names slices; one that does not exist is an error. */
    if (only == NULL) {
        for (uint32_t i = 0; i < count; i++) {
            slices[i].selected = 1;
        }
    }
    for (const char *q = only; q != NULL && *q != '\0';) {
        const char *comma = strchr(q, ',');
        size_t len = comma != NULL ? (size_t)(comma - q) : strlen(q);
        int found = 0;
        for (uint32_t i = 0; i < count; i++) {
            if (strlen(slices[i].name) == len && memcmp(slices[i].name, q, len) == 0) {
                slices[i].selected = 1;
                found = 1;
            }
        }
        if (!found) {
            fprintf(stderr, "vectors: %s: no file named %.*s\n", path, (int)len, q);
            finish(1);
        }
        q = comma != NULL ? comma + 1 : q + len;
    }

    make_instance();
    unsigned long ran = 0u, failures = 0u, absorbed = 0u;
    unsigned printed = 0u;
    for (uint32_t i = 0; i < count; i++) {
        if (slices[i].selected) {
            run_slice(&slices[i], flip_field, &ran, &failures, &absorbed, &printed);
        }
    }

    /* An allow line for a file this run covered must match a failing case. */
    int stale = 0;
    for (uint32_t i = 0; i < allow_n; i++) {
        for (uint32_t j = 0; j < count; j++) {
            if (slices[j].selected && strcmp(slices[j].name, allow[i].file) == 0 &&
                !allow[i].used) {
                fprintf(stderr, "vectors: allow line %s %s matched no failing case\n",
                        allow[i].file, allow[i].hex);
                stale = 1;
            }
        }
    }
    printf("vectors: %lu cases, %lu failures\n", ran, failures);
    if (absorbed != 0u) {
        printf("vectors: %lu cases failed and are allowed\n", absorbed);
    }
    if (ran < expect) {
        fprintf(stderr, "vectors: ran %lu cases, expected at least %lu\n", ran, expect);
    }
    finish(failures != 0u || ran < expect || stale ? 1 : 0);
}
