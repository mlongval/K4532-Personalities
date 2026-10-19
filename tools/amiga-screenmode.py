#!/usr/bin/env python3
"""amiga-screenmode.py FILE [DEPTH] -- an AmigaOS ScreenMode prefs file
(ENVARC:Sys/screenmode.prefs) for the Workbench screen: PAL high resolution,
640x256, DEPTH bitplanes (8: 256 colours, AGA), autoscroll.  Workbench 3.1's
own default is 4 colours, too few for Scalos's palette ("5 of the pens
defined in Scalos Palette prefs could not be allocated", Doc 2026-10-18) and
for colour icons.  The layout is prefs/screenmode.h's: a PRHD header and a
SCRM chunk (4 reserved longs, DisplayID, width, height, depth, control)."""
import struct, sys
path = sys.argv[1]; depth = int(sys.argv[2]) if len(sys.argv) > 2 else 8
PAL_HIRES = 0x00029000                       # PAL_MONITOR_ID | HIRES_KEY
prhd = b"PRHD" + struct.pack(">I", 6) + struct.pack(">BBI", 0, 0, 0)
scrm = struct.pack(">4IIHHHH", 0, 0, 0, 0, PAL_HIRES, 640, 256, depth, 1)   # SMF_AUTOSCROLL
scrm = b"SCRM" + struct.pack(">I", len(scrm)) + scrm
body = b"PREF" + prhd + scrm
open(path, "wb").write(b"FORM" + struct.pack(">I", len(body)) + body)
