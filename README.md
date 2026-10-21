# K4532 Personalities

Other machines for the K4532, picked at power-on like a MiSTer/MEGA65 core:
hold **SPACE** at power-on (the splash says so) and the Chooser lists the
Commodore 64 (and through its sidebar the 128), the PET, the Apple ][ and
the Commander X16.  The Amiga 1200 is sidelined (Doc, 2026-10-20): its code
stays here and its image on p4 in `personalities/sidelined/`, but it is not
built by default nor listed.
The chosen machine runs from RAM; quitting it (F12 -> Quit in VICE,
QUIT or Shift+F2 on the Apple, F12 -> Exit on the X16)
brings the Chooser back, and the K4532 is one of its choices.  No space:
the machine used last, one second later.  Software goes in through the
K4532 hub's Personalities section (files under `~/personalities/<name>`:
D64s, ADFs, the Amiga's DH0) or a USB stick.  (A GRUB-entry version came
first, but was dropped the same day: Doc wants Debian to show the choice.)
(Started 2026-10-08 as "could BMC64 run in a container?" — BMC64 itself is
Pi bare-metal glue around VICE 3.3; upstream VICE's SDL UI is the better fit.)

| Personality | Emulator | Image |
|---|---|---|
| c64, c128 (+16 MB REU, SD volume on drive 9), pet (4032, 40 columns) | VICE 3.10, SDL2 UI (x64sc, x128, xpet) | `vice.squashfs` ~5 MB |
| apple2 (the enhanced //e at first; ][, ][+, //e; Disk II in slot 6) | AppleWin's sa2 (Linux SDL2 frontend, github.com/audetto/AppleWin), plain renderer | `apple2.squashfs` ~3 MB |
| a1200, sidelined (KS 3.1, WB 3.1 on DH0, Work 20 MB on DH1; the A500 went 2026-10-12) | Amiberry 8.3.0, SDL3, no OpenGL | `amiga.squashfs` ~18 MB |
| x16 (Commander X16, ROM r49; drive 8 = `~/personalities/x16`) | x16emu r49, SDL2 | `x16.squashfs` ~0.2 MB |

Quitting back to the K4510: VICE and Amiberry F12 -> Quit; the X16 F12 ->
Exit to the K4510 (our menu: also Reset, Warp, Placement, Scale, Sidebar), Alt+F4, or
`POWEROFF` at its BASIC prompt; the K4532 F12 -> Machine -> Quit the emulator
(or Shift+Esc), as on the K4510 -- its Power off and Save and power off turn
the machine off, as the K4510's do.

**The K4532** is no longer a personality (2026-10-12, K4532's PLAN.md step
6): it is the machine itself, the layer's `~/k4532`, and the one these
personalities come back to.  Its image, `machine/k4532` and
`tools/build-k4532.sh` went; their history is in git.

**Placement** (all three emulators, build-time patches in `tools/patches/`):
`K4510_PLACEMENT=left|centre`, `K4510_SCALE=fit|integer`, defaults from
`~/personalities/display.cfg`, shared by all three: `placement=left|centre`,
`scale=fit|integer`, `sidebar=NAME`, `vdcborders=on|off` (starting file:
left, fit, antfarm). The X16's F12 menu writes the first three, the C128's
the last; each keeps the other lines. Left puts the picture flush left at
full height and leaves the rest of the width free for a **sidebar**: the
K4532's scenes (`~/k4532/sdl/libk4532side.so`, dlopened via
`tools/patches/common/k4510host.c`): none antfarm matrix space river
dreamfall tetris halloween christmas.

**C64, C128 and PET**, placement left: the sidebar is the machine's own panel
instead of a K4532 scene (`tools/patches/vice/k4510c64.c`), in its case's
colour -- the breadbin's brown beige, the C128's light beige -- and lettered
in its own character ROM: a 1541 (the C128: a 1571) whose red LED lights while
it works and whose head moves track by track, the disk's directory as
`LOAD"$",8` lists it, the SD volume, and the keys on the PC's.  It works with
the mouse (it draws its own pointer; VICE shows none in fullscreen): click a
program in the directory to load and run it, DISKS for the disks in
`~/personalities/<name>` and its folders (a disk goes in drive 8; a .PRG, .T64
or .CRT runs at once), EJECT to take the disk out, BROWSE for Dir Browser on
the SD volume; the wheel scrolls.  On the C128 the screen it is not showing
(VDC or VIC-II) is along the bottom of the sidebar, the panel above it.
The **PET** has the same panel in its screen's own two colours, read from
VICE's palette as it draws; its screen is **white on dark blue** at first,
and the panel's SCREEN row picks GREEN, AMBER, GREY or BLUE, with TOGGLE
going between blue and the last of the others (the choice is kept in
`~/personalities/pet/palette`, which run/vice passes after `-model 4032`:
the model sets the 4032's own palette, so a choice made only in F12 never
lasted a start; whiteblue.vpl and grey.vpl are in `machine/vice/PET`;
`tools/test-petscreen.sh`).  The PET's picture is its text area only,
the 25 rows and nothing above or below, stretched to the full height even
with `scale=integer` and drawn about 13:10, a PET tube's shape, so the panel keeps its size.  Every
sidebar (C64, C128, PET) has a MODEL row: the PET a 2001, 3032, 4032 or
8032, the C64 a C64, C64C, SX-64 or NTSC, the C128 a C128, C128DCR or NTSC;
a click powers the new one on, and `~/personalities/<name>/model` keeps it (`petborders=on` in display.cfg keeps the borders).  Its drive is whatever Drive8Type is (a 2031 at first; a
.D64 goes in a 2031, a .D80 in an 8050), its keys the graphics keyboard's, and no SD row.
`K4510_C64_PANEL=0` gives the scene back.  `tools/test-c64panel.sh` clicks
through it under Xvfb (xdotool) and reads VICE's log.

Both machines (run/vice): the keyboard by symbol (`-keymap 0`: a PC key types
what is printed on it; the PET too), a 16 MB REU (VICE's
largest), and drive 9 an **SD volume**: the folder `~/personalities/SD`,
shared, as VICE's filesystem device (which takes SD2IEC's `CD` commands), with
[Dir Browser](https://graham-it.itch.io/dirbrowser) on it as DB64 and DB128
(`tools/get-dirbrowser.sh` fetches its VICE builds; its terms do not allow
redistribution, so it is never in this repository, only in the image built
here).  The C64 powers on white on dark blue: the stock KERNAL with three
bytes changed (`make-images.sh`, kernal-k4532.bin).  The keymap and the REU
are defaults only: run/vice passes each one only when the machine's own
section of `~/.config/vice/sdl-vicerc` does not set it (F12 -> Settings ->
Save, or the K4532 hub's Settings form, which writes that same section).

**The C128** is not in the Chooser (Doc, 2026-10-20): the C64's MODEL row
ends in C128, which quits the C64 and starts the C128 (the C128's ends in
C64, back again) -- run/vice notes the other machine in
`~/personalities/switch` and starts it when VICE quits, and VICE quitting
without it ends the personality.

**Apple ][**, placement left: the sidebar is the Apple's own panel
(`tools/patches/apple2/k4510apple2.cpp`, built into AppleWin's sa2 by
`tools/build-apple2.sh`), in the ][+'s beige with the six-stripe apple and
lettered in its own character ROM: the Disk II with its IN USE light and
track, what is in drive 1 and drive 2, SCREEN (COLOR, or a GREEN, AMBER,
WHITE or BLUE -- white on light blue -- monitor; TOGGLE-INVERSE swaps ink and
paper on any of them), MODEL ( ][, ][+, //E, //E ENH: the machine starts
again as it), the disks in `~/personalities/apple2` (a click puts one in
drive 1 and boots it; DOS 3.3's System Master and ProDOS 2.4.3 to start
with, as AppleWin ships them), EJECT, RESET (Ctrl-Reset), QUIT, and the keys
(Ctrl+F2 Reset, F2 power off and on, the Alt keys the Open and Solid Apple,
F5 swaps the drives, F9 the next screen, F12 OPTIONS, Shift+F2 quits).
F12 opens OPTIONS over the picture: SCANLINES (off, soft, strong), PIXELS
(sharp or smoothed), DISK (fast or the real speed), SOUND (off, low, mid,
full), and the SCREEN, INVERSE and MODEL of the sidebar; arrows choose,
Return starts the machine as the MODEL chosen, F12 or Esc closes.  The
monitors and scanlines are the sidebar's, not AppleWin's: AppleWin draws
colour or a white monitor, and each frame is coloured on its way to the
screen.  The choices are kept in `~/personalities/apple2` (screen, inverse,
model, disk1, scanlines, pixels, disk, sound); AppleWin's own settings in
its applewin.yaml there.
`tools/test-apple2.sh` clicks through it (and OPTIONS) under Xvfb and reads
the log (the drive turning: DOS 3.3 and ProDOS boot).  The picture is 4:3 at full height.
(The K4510 had an Apple //e on its Tube, LinApple, until 2026-10-07.)

**Amiga 1200** (sidelined 2026-10-20), placement left: the sidebar is the A1200's own panel
(`tools/patches/amiberry/k4510amiga.c`), in its off-white and lettered in
Topaz 8 taken from the Kickstart (`tools/topaz.py`): the power, floppy and
hard-drive LEDs, the disk in DF0: with its track, DH0: and DH1:, how the
touchpad clicks, and the Amiga's keys on the PC's.  It only shows (the mouse
is the Amiga's), but for one key: **F11 switches the desktop** between
Workbench 3.1 and **Scalos** 41.7 (shown in the panel's Desktop box; it
takes effect at the next start or reset).  Scalos (the Scalos Team's last
68k beta) and PeterK's icon.library 51 (with RemLib, so Workbench 3.1 shows
NewIcons and colour icons) and MUI 3.8 (with the classes Scalos's Prefs needs: NList, BetterString,
TextEditor, Lamp, Urltext, Textinput, Popplaceholder, Pophotkey) are fetched
by `tools/get-scalos.sh` -- Scalos
from the Internet Archive's copy of its site, whose readme asks it be had
only from there, so it is never in this repository -- laid out by
`tools/install-scalos.sh` as its installer would, and copied onto DH0 by
run/amiga once; the Startup-Sequence runs Scalos's LoadWB (C:LoadWB.scalos)
while `S:Desktop-Scalos` is there, Workbench's own otherwise.
`tools/test-amiga-desktop.sh` boots both under Xvfb.  DH1: is **Work, a 20 MB hardfile** (FFS, made by
`tools/make-amiga-hd.sh` with amitools; copied to the machine only if none is
there; run/amiga adds it to a configuration written before it).  The
**touchpad** works as the mouse (`tools/patches/amiberry/touchpad.py`): on
the bare console SDL3 reports it as fingers, which Amiberry used only for a
finger id SDL3 never gives; now a finger moves the pointer, a tap clicks, two
fingers tap the right button, and a tap then a held finger drags.

**RTG** on the A1200: a 68020 with all 32 address lines (its own 68EC020 has
24, too few for a Zorro III board) and UAE's graphics card, uaegfx, 16 MB,
with Picasso96 2.0 on DH0 -- the installer's steps done on the host by
`make-images.sh` (`tools/amiga-icon.py` writes BoardType=uaegfx into the
monitor driver's icon) and copied onto DH0 by run/amiga once, the card added
to a configuration written before it (a `gfxcard_size` line of any kind is
left alone: 0 turns RTG off).  Prefs/ScreenMode then lists the uaegfx modes.
Picasso96 is shareware (US$20 asked of a user) and must be passed on only as
its whole archive: `tools/get-picasso96.sh` fetches it from Aminet into
work/, and it is never in this repository.  `tools/test-amiga-rtg.sh` boots
the A1200 under Xvfb and reads Amiberry's log for rtg.library starting the
card.

**C128**, placement left: the other display lives along the bottom of the
sidebar, below its panel, live (the
VDC with its borders cropped off: its 80x25 text area plus 8 pixels, so the
80-column text is readable), F12 -> Video settings -> VICII/VDC swaps them.
At power-on the screen the C128 did not start on is blank (black on black)
until something is printed to it.
C128 keys on the PC (symbolic, sdl_sym.vkm): Esc = RUN/STOP, F9 = ESC (so
ESC X = F9 then X, switching the BASIC screen), 40/80 DISPLAY = keypad `/`, or F12 -> Video settings -> 40/80 key
(latched; the C128 reads it at reset, so down + reset = 80 columns).
F12 -> Video settings -> VDC borders off: the VDC as the main display shows
the same cropped text area, scaled up (saved as `vdcborders=off` in
display.cfg). While the F12 menu is open the VDC is shown whole, borders and
all, because VICE lays the menu out on the whole frame (cropped, its top rows
were lost).
`K4510_C128_DUAL=0` / `K4510_VDC_CROP=0` turn the dual display / the crop off.

**Software** in `~/personalities/` (on p4, where each file browser starts):
the C64 library curated from Doc's collection by `tools/curate-c64.sh`
(GAMES DEMOS UTILS LANGUAGES DOCS; MANIFEST.txt says where each came from),
and Attack of the PETSCII Robots, the free shareware editions from the 8-Bit
Guy's site (`tools/get-robots.sh`), for the C64 (+REU), C128, PET, Amiga, X16.

VIC-20 and Plus/4 are sidelined (Doc, 2026-10-08).

## Build (ubuntu-s1, rootless podman)

    podman build -t personalities-builder -f tools/Containerfile tools
    podman build -t k4510-base-mimic -f tools/Containerfile.base tools   # trixie + K4510 packages.list
    podman build -t k4510-base-test  -f tools/Containerfile.test tools   # + Xvfb
    podman run --rm -v $PWD/work:/work:Z -v $PWD/tools:/tools:ro personalities-builder sh /tools/build-vice.sh
    podman run --rm -v $PWD/work:/work:Z -v $PWD/tools:/tools:ro personalities-builder sh /tools/build-amiberry.sh
    podman run --rm -v $PWD/work:/work:Z -v $PWD/tools:/tools:ro personalities-builder sh /tools/build-x16.sh
    podman run --rm -v $PWD/work:/work:Z -v $PWD/tools:/tools:ro personalities-builder sh /tools/build-apple2.sh
    tools/curate-c64.sh; tools/get-robots.sh   # software for ~/personalities
    tools/make-images.sh             # -> work/out/{personalities,home}: vice, x16, apple2 (make-images.sh amiga: the sidelined one alone)
    tools/test-personality.sh c64    # -> work/shots/c64.jpg (Xvfb on the base stand-in)
    tools/export-patches.sh          # -> patches/*.patch: the same changes as plain diffs (patches/README.md)
    tools/deploy-dell.sh             # images to p4, starting files to ~/personalities
    tools/deploy-dell.sh -n --only amiga   # what one family's deploy would copy, sending nothing

Libraries the K4532's Debian base lacks are found by asking `k4510-base-mimic`
(the container's name is still the old one) and bundled into each image's
`lib/`. Refresh `tools/k4510-base.packages` from the K4532 repo's
`linux/packages.list` when the base changes.

Two upstream bugs patched at build time: VICE 3.10 crashes at the banner when
stdout is not a terminal (`src/log.c`); Amiberry 8.3.0's non-OpenGL renderer
does not compile (`sdl_renderer.cpp`, a misnamed SDL3 type). And one change:
x16emu's fullscreen becomes FULLSCREEN_DESKTOP (the panel's own mode, as the
K4510 does) instead of a switch to a 640x480 mode the panel may not have.

## The K4532 side (the K4532 repo, linux/)

- `linux/config/includes.chroot/usr/local/bin/k4532-personality` — copy the image to /run, mount, run
- `linux/config/includes.chroot/usr/local/bin/k4532-session` — the loop: the machine used last, the Chooser after a quit, the K4532 if the Chooser fails
- `sdl/chooser.c` — the Chooser itself (SPACE held at power-on; `k4532.personality=` on the kernel command line picks one for a boot)
- `docs/STORAGE.md` §3b

## Where things live on the machine

- p4 `/personalities/*.squashfs` + `*.list` — the images (copied to RAM only when chosen)
  and `*.prev`, the ones before the last deploy. The RAM copy
  (`/run/k4510-personalities/<fam>.squashfs`, mounted at
  `/opt/personalities/<fam>`) stays until a reboot, so a new image deployed
  while the K4510 runs takes effect after a reboot, or after (machine closed)
  `sudo umount /opt/personalities/<fam>; sudo rm /run/k4510-personalities/<fam>.squashfs`.
  Copy to p4 on battery under `sudo -n systemd-inhibit`.
- `~/personalities/<name>/` — floppies, DH0, `.uae`; VICE settings in `~/.config/vice/vicerc`
- Kickstarts and Workbench are your own, named in `local.cfg` (copy
  `local.cfg.example`; it is not tracked): KS 1.3 (no longer used: the A500 went), KS 3.1 A1200
  crc 1483a091.  The C64 collection (`C64_COLLECTION`) and the machine to
  deploy to (`DEPLOY_HOST`) are named there too.

## License

MIT (`LICENSE`) for this repo's own code: the scripts, the sidebar glue
(`tools/patches/common/`) and the X16's F12 menu (`tools/patches/x16/k4510menu.*`).
The patches in `patches/` change VICE (GPL-2.0-or-later), Amiberry (GPL-3.0)
and the X16 emulator (BSD-2-Clause); a patched tree is under its project's
license.  No ROMs, Kickstarts or games are in this repo: the X16 ROM and the
PETSCII Robots shareware are downloaded from their official sources at build
time, and the Amiga and C64 files come from your own (`local.cfg`).
