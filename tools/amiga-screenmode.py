#!/usr/bin/env python3
"""amiga-screenmode.py FILE [DEPTH [DISPLAYID WIDTH HEIGHT]] -- an AmigaOS
ScreenMode prefs file (ENVARC:Sys/screenmode.prefs) for the Workbench
screen.  By default PAL high resolution, 640x256, DEPTH bitplanes (8: 256
colours, AGA); with a DISPLAYID (hex) and its size, that mode -- an RTG
one, say, 0x50031100 640 480 with DEPTH 15: uaegfx's 640x480 in 15-bit
colour.  Workbench 3.1's own default is 4 colours, too few for Scalos's
palette ("5 of the pens defined in Scalos Palette prefs could not be
allocated", Doc 2026-10-18) and for colour icons.  The layout is
prefs/screenmode.h's: a PRHD header and a SCRM chunk (4 reserved longs,
DisplayID, width, height, depth, control)."""
import struct, sys
a = sys.argv[1:]
path = a[0]; depth = int(a[1]) if len(a) > 1 else 8
mode, w, h = (int(a[2], 16), int(a[3]), int(a[4])) if len(a) > 4 else (0x00029000, 640, 256)   # PAL_MONITOR_ID | HIRES_KEY
prhd = b"PRHD" + struct.pack(">I", 6) + struct.pack(">BBI", 0, 0, 0)
scrm = struct.pack(">4IIHHHH", 0, 0, 0, 0, mode, w, h, depth, 1)   # SMF_AUTOSCROLL
scrm = b"SCRM" + struct.pack(">I", len(scrm)) + scrm
body = b"PREF" + prhd + scrm
open(path, "wb").write(b"FORM" + struct.pack(">I", len(body)) + body)
