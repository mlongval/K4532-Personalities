#!/bin/sh
# The A1200's RTG, under Xvfb, read from Amiberry's log -- no pictures:
#   the run script puts Picasso96 on DH0 and the graphics card in the
#   configuration; the Amiga boots Workbench from DH0, its monitor driver
#   (Devs/Monitors/uaegfx) opens rtg.library, and rtg.library finds and
#   starts the card: Amiberry logs uaegfx.card's init and then InitCard's
#   "P96: ..." lines, which only an AmigaOS side that has found the board
#   reaches.
#   tools/test-amiga-rtg.sh      (after tools/make-images.sh amiga)
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd); W=$HERE/work
rm -rf $W/test-amiga $W/test-ahome
unsquashfs -q -d $W/test-amiga $W/out/personalities/amiga.squashfs
mkdir -p $W/test-ahome/personalities && cp -a $W/out/home/personalities/a1200 $W/test-ahome/personalities/
rm -f $W/test-ahome/personalities/a1200/a1200.uae
rm -rf $W/test-ahome/personalities/a1200/DH0/Libs/Picasso96      # as on a machine from before RTG
podman run --rm -v $W/test-amiga:/opt/personalities/amiga:ro,Z -v $W/test-ahome:/home/k4510:Z \
    -e HOME=/home/k4510 -e SDL_AUDIODRIVER=dummy -e SECS k4510-base-test sh -c '
    Xvfb :1 -screen 0 1920x1080x24 >/dev/null 2>&1 & sleep 1; export DISPLAY=:1
    /opt/personalities/amiga/run/a1200 --log >~/rtg.log 2>&1 & sleep ${SECS:-60}
    kill $! 2>/dev/null; sleep 1
    grep -a "^cpu_type\|^cpu_24bit\|^gfxcard" ~/personalities/a1200/a1200.uae
    ls ~/personalities/a1200/DH0/Libs/Picasso96 ~/personalities/a1200/DH0/Devs/Monitors
    grep -a -i "uaegfx\|P96\|picasso\|rtg" ~/rtg.log | sort | uniq -c | head -30
    grep -aq "uaegfx.card .* init" ~/rtg.log || { tail -40 ~/rtg.log; echo "test-amiga-rtg: FAILED: no uaegfx card"; exit 1; }
    grep -aq "P96: " ~/rtg.log || { tail -40 ~/rtg.log; echo "test-amiga-rtg: FAILED: Picasso96 never started the card"; exit 1; }
    echo "test-amiga-rtg: OK (Picasso96 on DH0, the card configured, rtg.library started uaegfx)"'
