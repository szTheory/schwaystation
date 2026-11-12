#!/usr/bin/env python3
"""Compare the hosted screenshots and adapt this run's pinned BSV sample."""
import struct
import sys
import tempfile
import io
from contextlib import redirect_stdout
from pathlib import Path

BMP_LIMIT = 16 * 1024 * 1024
DIM_LIMIT = 16384
MOVIE_LIMIT = 16 * 1024 * 1024
CHECKPOINT_LIMIT = 64 * 1024 * 1024
MOVIE_FRAMES = 59
RECORD_SIZE = 264


def require(ok, message):
    if not ok:
        raise ValueError(message)


def bmp(path):
    path = Path(path)
    require(path.stat().st_size <= BMP_LIMIT, f"BMP exceeds size limit: {path}")
    data = path.read_bytes()
    require(70 <= len(data) <= BMP_LIMIT and data[:2] == b"BM", f"not a bounded BMP: {path}")
    offset, header = struct.unpack_from("<II", data, 10)
    width, signed_height = struct.unpack_from("<ii", data, 18)
    planes, depth, compression = struct.unpack_from("<HHI", data, 26)
    rows = abs(signed_height)
    require(header in (40, 108, 124) and planes == 1, f"unsupported BMP header: {path}")
    require(0 < width <= DIM_LIMIT and 0 < rows <= DIM_LIMIT, f"invalid BMP dimensions: {path}")
    require(offset >= 14 + header and offset <= len(data), f"invalid BMP data offset: {path}")
    masks = depth == 32 and compression == 3
    if masks:
        require(offset >= 66, f"truncated BMP channel masks: {path}")
        require(struct.unpack_from("<III", data, 54) == (0xFF0000, 0xFF00, 0xFF),
                f"unsupported BMP channel masks: {path}")
    require(masks or (depth in (24, 32) and compression == 0), f"unsupported BMP pixels: {path}")
    stride = ((width * depth + 31) // 32) * 4
    require(offset + stride * rows <= len(data), f"truncated BMP pixels: {path}")
    rgb = bytearray(width * rows * 3)
    top_down = signed_height < 0
    for y in range(rows):
        source = offset + stride * (y if top_down else rows - 1 - y)
        for x in range(width):
            p = source + x * (depth // 8)
            q = (y * width + x) * 3
            rgb[q:q + 3] = data[p + 2], data[p + 1], data[p]
    return data, width, rows, depth, offset, stride, top_down, bytes(rgb)


def compare(expected, actual, size=None):
    _, ew, eh, _, _, _, _, ergb = bmp(expected)
    _, aw, ah, _, _, _, _, argb = bmp(actual)
    wanted = size or (ew, eh)
    if (aw, ah) != wanted or (ew, eh) != wanted:
        print(f"size differs: expected {wanted[0]}x{wanted[1]}, actual {aw}x{ah}")
        return 1
    for pos in range(0, len(ergb), 3):
        if ergb[pos:pos + 3] != argb[pos:pos + 3]:
            pixel = pos // 3
            x, y = pixel % ew, pixel // ew
            left = int.from_bytes(ergb[pos:pos + 3], "big")
            right = int.from_bytes(argb[pos:pos + 3], "big")
            print(f"first difference at {x},{y}: expected {left:06X}, actual {right:06X}")
            return 1
    print(f"{ew}x{eh}, every pixel equal")
    return 0


def pixel_control(path, x, y):
    data, width, height, depth, offset, stride, top_down, _ = bmp(path)
    require(0 <= x < width and 0 <= y < height, "pixel control is outside the image")
    row = y if top_down else height - 1 - y
    pos = offset + row * stride + x * (depth // 8)
    changed = bytearray(data)
    changed[pos] ^= 0xFF
    with tempfile.NamedTemporaryFile(dir=Path(path).parent, suffix=".bmp") as out:
        out.write(changed)
        out.flush()
        message = io.StringIO()
        with redirect_stdout(message):
            result = compare(path, out.name)
    print(f"pixel control: compare exited {result}: {message.getvalue().strip()}")
    require(result == 1 and message.getvalue().startswith(f"first difference at {x},{y}: "),
            "pixel mutation was not rejected at the expected coordinate")


def dimension_control(path, width, height):
    message = io.StringIO()
    with redirect_stdout(message):
        result = compare(path, path, (width, height))
    output = message.getvalue().strip()
    print(f"dimension control: compare exited {result}: {output}")
    require(result == 1 and output.startswith("size differs:"), "wrong dimensions were accepted")


def distinct(left, right):
    _, lw, lh, _, _, _, _, lp = bmp(left)
    _, rw, rh, _, _, _, _, rp = bmp(right)
    require((lw, lh) == (rw, rh), "frame geometry differs")
    require(lp != rp, "captured frames are identical")
    print(f"{lw}x{lh}, frames are distinct")


def patch_movie(source, destination):
    source = Path(source)
    require(54 <= source.stat().st_size <= MOVIE_LIMIT, "BSV size is outside the bound")
    data = bytearray(source.read_bytes())
    require(data[:4] == b"2VSB" and struct.unpack_from("<I", data, 4)[0] == 2,
            "expected RetroArch replay v2")
    require(20 <= struct.unpack_from("<I", data, 12)[0] <= CHECKPOINT_LIMIT,
            "invalid replay state size")
    require(struct.unpack_from("<I", data, 20)[0] == 0 and
            struct.unpack_from("<I", data, 24)[0] == MOVIE_FRAMES and not any(data[28:40]),
            "unexpected replay header fields")
    require(data[40:42] == b"\x02\x00", "unexpected checkpoint codec")
    raw_size, packed_size, checkpoint_size = struct.unpack_from("<III", data, 42)
    require(0 < raw_size <= CHECKPOINT_LIMIT and 0 < packed_size <= CHECKPOINT_LIMIT and
            0 < checkpoint_size <= CHECKPOINT_LIMIT, "invalid checkpoint lengths")
    records = 54 + checkpoint_size
    require(records + MOVIE_FRAMES * RECORD_SIZE == len(data), "unexpected replay record bounds")
    target = None
    ids = {0: set(), 1: set()}
    for frame in range(MOVIE_FRAMES):
        start = records + frame * RECORD_SIZE
        record = data[start:start + RECORD_SIZE]
        require(record[4:7] == b"\x00\x20\x00" and record[-1:] == b"f",
                f"invalid replay frame header {frame + 1}")
        expected_backref = 0 if frame == 0 else RECORD_SIZE
        require(struct.unpack_from("<I", record)[0] == expected_backref,
                f"invalid replay checkpoint back-reference {frame + 1}")
        cross = 0
        for item in range(32):
            event = start + 7 + item * 8
            port, device, index, reserved = data[event:event + 4]
            ident = struct.unpack_from("<H", data, event + 4)[0]
            value = struct.unpack_from("<h", data, event + 6)[0]
            require(port in (0, 1) and device == 1 and index == 0 and reserved == 0,
                    f"unexpected joypad event in frame {frame + 1}")
            require(ident < 16 and ident not in ids[port], f"duplicate joypad id in frame {frame + 1}")
            ids[port].add(ident)
            require(value == 0, f"recording already has pressed input in frame {frame + 1}")
            if port == 0 and ident == 0:
                cross += 1
                if frame == MOVIE_FRAMES - 1:
                    target = event + 6
        require(cross == 1, f"expected one port-0 Cross event in frame {frame + 1}")
        require(ids == {0: set(range(16)), 1: set(range(16))},
                f"incomplete joypad event set in frame {frame + 1}")
        ids = {0: set(), 1: set()}
    require(target is not None and data[target:target + 2] == b"\x00\x00", "Cross was not released")
    changed = bytearray(data)
    changed[target:target + 2] = b"\x01\x00"
    differences = [i for i, (before, after) in enumerate(zip(data, changed)) if before != after]
    require(differences == [target], "replay edit changed bytes outside final Cross value")
    Path(destination).write_bytes(changed)
    print(f"replay v2: changed only frame {MOVIE_FRAMES} Cross from 0 to 1")


def main(args):
    require(bool(args), "usage: verify.py compare|pixel-control|dimension-control|distinct|patch ...")
    if args[0] == "compare" and len(args) == 5:
        return compare(args[1], args[2], (int(args[3]), int(args[4])))
    if args[0] == "pixel-control" and len(args) == 4:
        pixel_control(args[1], int(args[2]), int(args[3]))
    elif args[0] == "dimension-control" and len(args) == 4:
        dimension_control(args[1], int(args[2]), int(args[3]))
    elif args[0] == "distinct" and len(args) == 3:
        distinct(args[1], args[2])
    elif args[0] == "patch" and len(args) == 3:
        patch_movie(args[1], args[2])
    else:
        raise ValueError("usage: verify.py compare|pixel-control|dimension-control|distinct|patch ...")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except (OSError, struct.error, ValueError) as error:
        sys.exit(f"verify: {error}")
