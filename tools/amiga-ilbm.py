#!/usr/bin/env python3
"""amiga-ilbm.py IN OUT [WIDTH HEIGHT] -- any picture PIL reads (PNG, JPEG)
as a 24-bit IFF ILBM, cut to the size's shape from the middle and scaled to
it (default 640x480): what the A1200's picture.datatype 43 and ilbm.datatype
44 show on a Hi/TrueColour screen without a PNG or JPEG datatype -- Scalos's
desktop picture.  Rows ByteRun1-packed, planes R0..R7 G0..G7 B0..B7."""
import struct, sys
from PIL import Image
src, dst = sys.argv[1], sys.argv[2]
W, H = (int(sys.argv[3]), int(sys.argv[4])) if len(sys.argv) > 4 else (640, 480)
im = Image.open(src).convert("RGB")
w, h = im.size
if w * H > h * W:                                # wider than the shape: the middle
    nw = h * W // H; im = im.crop(((w - nw) // 2, 0, (w - nw) // 2 + nw, h))
else:
    nh = w * H // W; im = im.crop((0, (h - nh) // 2, w, (h - nh) // 2 + nh))
im = im.resize((W, H), Image.LANCZOS)
px = im.tobytes()
rb = (W + 15) // 16 * 2
def packbits(row):
    out = bytearray(); i = 0; n = len(row)
    while i < n:
        j = i
        while j + 1 < n and row[j + 1] == row[i] and j - i < 127: j += 1
        if j > i:
            out += bytes([257 - (j - i + 1), row[i]]); i = j + 1
        else:
            j = i
            while j + 1 < n and j - i < 127 and not (j + 2 < n and row[j + 1] == row[j + 2]): j += 1
            out += bytes([j - i]) + row[i:j + 1]; i = j + 1
    return bytes(out)
body = bytearray()
for y in range(H):
    line = px[y * W * 3:(y + 1) * W * 3]
    for c in range(3):
        for bit in range(8):
            row = bytearray(rb)
            for x in range(W):
                if line[x * 3 + c] >> bit & 1: row[x >> 3] |= 0x80 >> (x & 7)
            body += packbits(bytes(row))
bmhd = struct.pack(">HHhhBBBBHBBhh", W, H, 0, 0, 24, 0, 1, 0, 0, 10, 11, W, H)
def chunk(t, d): return t + struct.pack(">I", len(d)) + d + (b"\0" if len(d) & 1 else b"")
form = b"ILBM" + chunk(b"BMHD", bmhd) + chunk(b"BODY", bytes(body))
open(dst, "wb").write(b"FORM" + struct.pack(">I", len(form)) + form)
