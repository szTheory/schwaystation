/* API version 2, as first released. FROZEN: once released this file never
   changes, except that the version check is a lower bound because a later
   API version keeps every version-2 name. It calls every function added in
   version 2, assigns every field and checks every constant it added, so a
   rename, a removal or a changed value fails to build or fails here. */
#include <string.h>

#include "schwaystation.h"
#include "../check.h"

static void put32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

int main(void)
{
    CHECK(SCHWAY_API_VERSION >= 2);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_EXE_INVALID, 11);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_EXE_RANGE, 12);
    CHECK_EQ_U64(SCHWAY_LICENCE_NONE, 0);
    CHECK_EQ_U64(SCHWAY_LICENCE_JAPAN, 1);
    CHECK_EQ_U64(SCHWAY_LICENCE_EUROPE, 2);
    CHECK_EQ_U64(SCHWAY_LICENCE_NORTH_AMERICA, 3);
    CHECK_EQ_U64(SCHWAY_EXE_LINT_LOW_64K, 1);
    CHECK_EQ_U64(SCHWAY_EXE_LINT_TRAILING, 2);
    for (schway_status st = SCHWAY_STATUS_ERR_EXE_INVALID; st <= SCHWAY_STATUS_ERR_EXE_RANGE;
         st++) {
        const char *text = schway_status_string(st);
        CHECK(text != NULL && text[0] != '\0');
        CHECK(strcmp(text, schway_status_string(SCHWAY_STATUS_ERR_EXE_RANGE + 1u)) != 0);
    }

    /* A PS-EXE header and one text sector: entry 80010000h, global pointer
       80020000h, text at 80010000h, stack at 801FFF00h. */
    static unsigned char exe[0x1000];
    memset(exe, 0, sizeof exe);
    memcpy(exe, "PS-X EXE", 8);
    put32(exe + 0x10, 0x80010000u);
    put32(exe + 0x14, 0x80020000u);
    put32(exe + 0x18, 0x80010000u);
    put32(exe + 0x1C, 0x800u);
    put32(exe + 0x20, 0x11u);
    put32(exe + 0x24, 0x22u);
    put32(exe + 0x28, 0x33u);
    put32(exe + 0x2C, 0x44u);
    put32(exe + 0x30, 0x801FFF00u);
    put32(exe + 0x34, 0u);
    memcpy(exe + 0x4C, "Sony Computer Entertainment Inc. for Europe area", 48);

    schway_exe_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    info.pc0 = 0u;
    info.gp0 = 0u;
    info.t_addr = 0u;
    info.t_size = 0u;
    info.d_addr = 0u;
    info.d_size = 0u;
    info.b_addr = 0u;
    info.b_size = 0u;
    info.s_addr = 0u;
    info.s_size = 0u;
    info.licence_area = 0u;
    info.lint = 0u;
    info.hash_len = 0u;
    CHECK_EQ_U64(schway_probe_exe(exe, sizeof exe, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(info.pc0, 0x80010000u);
    CHECK_EQ_HEX(info.gp0, 0x80020000u);
    CHECK_EQ_HEX(info.t_addr, 0x80010000u);
    CHECK_EQ_HEX(info.t_size, 0x800u);
    CHECK_EQ_HEX(info.d_addr, 0x11u);
    CHECK_EQ_HEX(info.d_size, 0x22u);
    CHECK_EQ_HEX(info.b_addr, 0x33u);
    CHECK_EQ_HEX(info.b_size, 0x44u);
    CHECK_EQ_HEX(info.s_addr, 0x801FFF00u);
    CHECK_EQ_HEX(info.s_size, 0u);
    CHECK_EQ_U64(info.licence_area, SCHWAY_LICENCE_EUROPE);
    CHECK_EQ_U64(info.lint, 0);
    CHECK_EQ_HEX(info.hash_len, 0x1000u);

    CHECK_EQ_U64(schway_probe_exe(NULL, sizeof exe, &info), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_probe_exe(exe, 0u, &info), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_probe_exe(exe, sizeof exe, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    exe[0] = 'X';
    CHECK_EQ_U64(schway_probe_exe(exe, sizeof exe, &info), SCHWAY_STATUS_ERR_EXE_INVALID);
    CHECK_DONE();
}
