#!/usr/bin/env python3
"""amiga-icon.py -- an Amiga .info file's tool types, read or set on the host.

    amiga-icon.py ICON.info                  print its tool types
    amiga-icon.py ICON.info OUT.info T=V ...  OUT: the same icon, these tool types

Written for Picasso96's monitor driver (make-images.sh): the installer copies
Devs/Monitors/Picasso96 as Devs/Monitors/uaegfx and sets BoardType=uaegfx in
its icon, which the driver reads; this does that without an Amiga.  A tool
type in brackets, "(SoftSprite=Yes)", is one that is off, as the installer
writes them.

The layout (AmigaOS 3.1 workbench/workbench.h): the DiskObject (78 bytes),
the DrawerData (56) if it has one, the image and the selected image (an Image,
20 bytes, then its planes), the default tool, the tool types (a count, then
each string with its length), the tool window; anything after (a drawer's
second DrawerData, NewIcons, GlowIcons) is kept as it is.
"""
import struct, sys


def strings_at(b, o, n):
    out = []
    for _ in range(n):
        ln = struct.unpack_from(">I", b, o)[0]; o += 4
        out.append(b[o:o + ln].rstrip(b"\0").decode("latin1")); o += ln
    return out, o


def parse(b):
    if struct.unpack_from(">H", b, 0)[0] != 0xE310:
        sys.exit("amiga-icon.py: not an icon (no E310)")
    sel_render = struct.unpack_from(">I", b, 4 + 26)[0]
    deftool, tooltypes, _, _, drawer, toolwin, _ = struct.unpack_from(">7I", b, 50)
    o = 78 + (56 if drawer else 0)
    for present in (True, bool(sel_render)):
        if not present:
            continue
        _, _, w, h, depth = struct.unpack_from(">5h", b, o)
        o += 20 + ((w + 15) // 16) * 2 * h * depth
    tt_at = o
    if deftool:
        ln = struct.unpack_from(">I", b, o)[0]; o += 4 + ln
    tt_start = o
    tts = []
    if tooltypes:
        n = struct.unpack_from(">I", b, o)[0] // 4 - 1; o += 4
        tts, o = strings_at(b, o, n)
    tt_end = o
    return {"tooltypes": tts, "tt_start": tt_start, "tt_end": tt_end, "deftool_at": tt_at}


def build(b, info, tts):
    blob = struct.pack(">I", (len(tts) + 1) * 4)
    for t in tts:
        s = t.encode("latin1") + b"\0"
        blob += struct.pack(">I", len(s)) + s
    head = bytearray(b[:info["tt_start"]])
    struct.pack_into(">I", head, 54, 1 if tts else 0)      # do_ToolTypes: present or not
    return bytes(head) + (blob if tts else b"") + b[info["tt_end"]:]


def main():
    b = open(sys.argv[1], "rb").read()
    info = parse(b)
    if len(sys.argv) == 2:
        print("\n".join(info["tooltypes"]))
        return
    tts = list(info["tooltypes"])
    for arg in sys.argv[3:]:
        key = arg.lstrip("(").split("=")[0].upper()
        tts = [t for t in tts if t.lstrip("(").split("=")[0].rstrip(")").upper() != key]
        tts.append(arg)
    open(sys.argv[2], "wb").write(build(b, info, tts))
    if parse(open(sys.argv[2], "rb").read())["tooltypes"] != tts:
        sys.exit("amiga-icon.py: the written icon does not read back")


if __name__ == "__main__":
    main()
