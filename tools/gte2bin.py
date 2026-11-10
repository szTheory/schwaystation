#!/usr/bin/env python3
"""Maintainer tool, never run by CI: transcode the public GTE hardware cases into one binary table.

usage: python3 -I tools/gte2bin.py TESTS_C OUT_BIN [--inc OUT_INC]
TESTS_C is gte/test-all/tests.c from JaCzekanski/ps1-tests at the commit named in pins.txt
(row ps1tests-gte-test-all); the tool refuses any other file. OUT_BIN is
tests/vectors/gte/ps1tests-1150.bin, all little-endian: a u32 case count, then per case 516 bytes,
a u32 opcode (FFFFFFFFh for a register-only case, else the 25-bit command field), 64 input words
and 64 output words, the registers 0 to 63 as written and as read back. Case names are dropped:
case N, counted from 1, is the upstream "test N". --inc writes tests/fixtures/src/gte_cases.inc.
The output holds no timestamp and no path, so two runs give the same bytes.
"""
import argparse
import hashlib
import re
import struct
import sys

BLOB = 'a03aa72173f122bf916039f438c8cde89e61e526'
COMMIT = 'f727802fead11f1daa7549285548392ef87749cb'
CASES = 1150
REG_ONLY = 0xFFFFFFFF
COMMANDS = (0x01, 0x06, 0x0C, 0x10, 0x11, 0x12, 0x13, 0x14, 0x16, 0x1B, 0x1C,
            0x1E, 0x20, 0x28, 0x29, 0x2A, 0x2D, 0x2E, 0x30, 0x3D, 0x3E, 0x3F)
PER_KIND = 50
INC_TOTAL = 26
WORD = re.compile(r'0x([0-9a-fA-F]{1,8})')


def blob_sha1(data):
    return hashlib.sha1(b'blob %d\0' % len(data) + data).hexdigest()


def parse(text):
    start = text.index('const struct test_t tests[] = {')
    body = text[start:text.index('\n};', start)]
    cases = []
    for entry in body.split('{ // test ')[1:]:
        op = int(WORD.search(entry[entry.index('.opcode'):]).group(1), 16)
        parts = {}
        for key in ('.input', '.output'):
            at = entry.index(key + ' = {')
            block = entry[at:entry.index('}', at)]
            parts[key] = [int(w, 16) for w in WORD.findall(block)]
            assert len(parts[key]) == 64, 'a case does not hold 64 %s words' % key
        cases.append((op if op == REG_ONLY else op & 0x1FFFFFF, parts['.input'], parts['.output']))
    return cases


def check_histogram(cases):
    assert len(cases) == CASES, '%d cases, want %d' % (len(cases), CASES)
    count = {}
    for op, _, _ in cases:
        key = 'reg' if op == REG_ONLY else op & 0x3F
        count[key] = count.get(key, 0) + 1
    want = {'reg': PER_KIND}
    want.update({c: PER_KIND for c in COMMANDS})
    assert count == want, 'case histogram differs: %r' % count


def select(cases):
    """Per command in ascending order the first case whose FLAG has bit 31, else a non-zero FLAG, else the first;
    then the first register-only cases up to INC_TOTAL in all. Numbers are upstream test numbers."""
    def flag(n):
        return cases[n - 1][2][63]
    picked = []
    for cmd in COMMANDS:
        group = [n for n, c in enumerate(cases, 1) if c[0] != REG_ONLY and c[0] & 0x3F == cmd]
        picked.append(next((n for n in group if flag(n) >> 31), None)
                      or next((n for n in group if flag(n)), group[0]))
    regs = [n for n, c in enumerate(cases, 1) if c[0] == REG_ONLY]
    return picked + regs[:INC_TOTAL - len(picked)]


def write_inc(path, cases):
    lines = ['; Hardware-captured GTE cases for the gte_vectors fixture, generated, do not edit.',
             '; Upstream: https://github.com/JaCzekanski/ps1-tests, gte/test-all/tests.c at commit',
             '; ' + COMMIT + ' (MIT, see THIRD-PARTY-NOTICES.md).',
             '; Transcoded by tools/gte2bin.py; values unchanged. Regenerate with:',
             ';   python3 -I tools/gte2bin.py TESTS_C tests/vectors/gte/ps1tests-1150.bin'
             ' --inc tests/fixtures/src/gte_cases.inc',
             '; Selection, fixed in the tool: for each of the 22 commands in ascending order the first case whose',
             '; output FLAG has bit 31 set, else the first with a non-zero FLAG, else the first; then the first',
             '; register-only cases to a total of %d. No case is invented.' % INC_TOTAL,
             '; A record is 66 words: the upstream case number, the opcode (FFFFFFFFh: registers only) and the 64',
             '; input words, registers 0 to 63.',
             '.definelabel GTE_CASE_COUNT, %d' % INC_TOTAL, 'gte_cases:']
    for n in select(cases):
        op, inputs, _ = cases[n - 1]
        words = [n, op] + inputs
        lines.append('; test %d, %s' % (n, 'registers only' if op == REG_ONLY else 'command %02Xh' % (op & 0x3F)))
        for i in range(0, 66, 8):
            lines.append('.word ' + ', '.join('0x%08X' % w for w in words[i:i + 8]))
    with open(path, 'w', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('tests_c')
    ap.add_argument('out_bin')
    ap.add_argument('--inc')
    args = ap.parse_args()
    with open(args.tests_c, 'rb') as f:
        data = f.read()
    if blob_sha1(data) != BLOB:
        sys.exit('gte2bin: %s is not the pinned blob %s' % (args.tests_c, BLOB))
    cases = parse(data.decode('utf-8'))
    check_histogram(cases)
    out = struct.pack('<I', len(cases))
    for op, inputs, outputs in cases:
        out += struct.pack('<129I', op, *inputs, *outputs)
    with open(args.out_bin, 'wb') as f:
        f.write(out)
    if args.inc:
        write_inc(args.inc, cases)
    print(len(cases), 'cases,', len(out), 'bytes')


if __name__ == '__main__':
    main()
