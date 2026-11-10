#!/usr/bin/env python3
"""Maintainer tool, never run by CI: slice the public CPU vectors into one container.

usage: python3 -I tools/sst_slice.py DIR OUT
DIR holds the upstream v1/*.json.bin files (SingleStepTests/r3000, see
tests/fixtures/manifest.txt for the pin); OUT is tests/vectors/r3000/sst-sample.bin.
Container, all little-endian: u16 slice count; per slice, in byte-sorted name
order, a 16-byte NUL-padded name (the file stem), a u32 byte length and the
slice. A slice is the upstream bytes of a file's first 100 records, with the
leading u32 case count patched to 100. Each record is 51 name bytes, u32 opcode
and address, two 172-byte states, u32 cycle count and 24 bytes per cycle.
"""
import os
import struct
import sys

KEEP = 100
HEAD = 51 + 8 + 172 * 2  # bytes before a record's cycle count


def slice_file(data):
    n = struct.unpack_from('<I', data, 0)[0]
    pos = 4
    for _ in range(KEEP):
        cycles = struct.unpack_from('<I', data, pos + HEAD)[0]
        pos += HEAD + 4 + 24 * cycles
    assert KEEP <= n and pos <= len(data)
    return struct.pack('<I', KEEP) + data[4:pos]


def main(src, out):
    names = sorted(f[:-9] for f in os.listdir(src) if f.endswith('.json.bin'))
    blob = struct.pack('<H', len(names))
    for name in names:
        with open(os.path.join(src, name + '.json.bin'), 'rb') as f:
            part = slice_file(f.read())
        blob += name.encode('ascii').ljust(16, b'\0') + struct.pack('<I', len(part)) + part
    with open(out, 'wb') as f:
        f.write(blob)
    print(len(names), 'slices,', len(blob), 'bytes')


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
