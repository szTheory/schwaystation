#!/usr/bin/env python3
"""Exercise deterministic fields in generated disc media."""

import importlib.util
import pathlib
import sys
import tempfile


sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("schwaystation_gen_disc", sys.argv[1])
if spec is None or spec.loader is None:
    raise SystemExit("could not load the disc media generator")
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)

sector = bytearray(generator.XA_SECTOR_BYTES)
sector[:8] = bytes((7, 3, 0x64, 0xF5, 7, 3, 0x64, 0xF5))
expected = bytearray(sector)
expected[3] = expected[7] = 1

with tempfile.TemporaryDirectory() as temporary:
    path = pathlib.Path(temporary) / "xa.xa"
    path.write_bytes(sector)
    generator.normalize_generated_xa_coding(path)
    if path.read_bytes() != expected:
        raise SystemExit("XA normalization changed bytes other than the duplicate coding fields")

    mismatch = bytearray(sector)
    mismatch[7] ^= 1
    path.write_bytes(mismatch)
    try:
        generator.normalize_generated_xa_coding(path)
    except ValueError:
        if path.read_bytes() != mismatch:
            raise SystemExit("XA normalization modified a sector with mismatched coding copies")
    else:
        raise SystemExit("XA normalization accepted mismatched coding copies")
