#!/usr/bin/env python3
"""amiga-icon-swap.py NEW OLD OUT -- a drawer icon's picture from NEW (a
colour icon, say) with OLD's window: OLD's DrawerData (where the drawer's
window opens and how large, made for this screen) put into NEW in place of
its own, and NEW's place on its parent left to Workbench (NO_ICON_POSITION),
so an icon made for another screen opens no window off this one.  Both must
be drawer-type DiskObjects (WBDRAWER/WBDISK/WBGARBAGE, with DrawerData)."""
import struct, sys
new, old = bytearray(open(sys.argv[1], "rb").read()), open(sys.argv[2], "rb").read()
for b, n in ((new, sys.argv[1]), (old, sys.argv[2])):
    if b[:2] != b"\xe3\x10" or struct.unpack(">I", b[66:70])[0] == 0:
        sys.exit(f"amiga-icon-swap: {n}: not a DiskObject with DrawerData")
new[78:78 + 56] = old[78:78 + 56]                 # struct DrawerData: NewWindow + dd_CurrentX/Y
new[58:66] = struct.pack(">ii", -2147483648, -2147483648)   # do_CurrentX/Y = NO_ICON_POSITION
open(sys.argv[3], "wb").write(new)
