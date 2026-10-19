#!/bin/sh
# The PET's screen colours (Doc, 2026-10-18), under Xvfb, checked in VICE's
# log -- no pictures: white on dark blue at the first start; the sidebar's
# GREEN, AMBER, GREY and BLUE change the palette, TOGGLE goes between blue
# and the last of the others, and ~/personalities/pet/palette keeps the
# choice for the next start.
#   tools/test-petscreen.sh      (after tools/make-images.sh vice; needs k4510-base-xdo)
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd); W=$HERE/work
rm -rf $W/test-vice $W/test-pethome
unsquashfs -q -d $W/test-vice $W/out/personalities/vice.squashfs
mkdir -p $W/test-pethome/personalities/pet
podman run --rm -v $W/test-vice:/opt/personalities/vice:ro,Z -v $W/test-pethome:/home/k4510:Z \
    -e HOME=/home/k4510 -e SDL_AUDIODRIVER=dummy -e K4510_PLACEMENT=left k4510-base-xdo sh -c '
V=/opt/personalities/vice; export LD_LIBRARY_PATH=$V/lib
Xvfb :1 -screen 0 1920x1080x24 >/dev/null 2>&1 & sleep 1; export DISPLAY=:1
ln -s $V/run/vice /tmp/pet
fail() { cat ~/log; echo "test-petscreen: FAILED: $1"; exit 1; }
start() {
    stdbuf -oL /tmp/pet -sounddev dummy +CRTCfull >~/log 2>&1 & PID=$!
    sleep 5; w=$(xdotool search --name VICE | head -1); xdotool windowmove $w 0 0 windowsize $w 1920 1080
    for i in $(seq 15); do grep -q "k4510c64: the panel" ~/log && break; sleep 1; done; sleep 2
    set -- $(sed -n "s/.*k4510c64: the panel at \([0-9]*\),\([0-9]*\), \([0-9]*\)x\([0-9]*\) machine pixels x\([0-9]*\).*/\1 \2 \3 \4 \5/p" ~/log | tail -1)
    [ -n "$5" ] || fail "no panel"
    X=$1 Y=$2 K=$5
}
at() { xdotool mousemove $((X + ($1 * 8 + 4) * K)) $((Y + ($2 * 8 + 4) * K)); sleep 0.3; xdotool click 1; sleep 1; }
# (VICE draws a palette through its colour settings, so the values are its
# adjusted ones: checked by which of red, green and blue leads)
colours() { grep "the PET.s colours" ~/log | tail -1 | sed "s/.*colours //"; }
kind() {                                              # "fg on bg" -> white-on-blue, green, amber, grey, other
    set -- $(colours); f=$1 b=$3
    fr=$((0x${f%????})) fg=$((0x${f#??})); fg=$((fg >> 8)); fb=$((0x${f#????})); br=$((0x${b%????})) bb=$((0x${b#????}))
    if [ $fr -gt 240 ] && [ $fg -gt 240 ] && [ $fb -gt 240 ] && [ $bb -gt $((br + 40)) ]; then echo white-on-blue
    elif [ $b != 000000 ]; then echo other
    elif [ $fg -gt $((fr + 60)) ]; then echo green
    elif [ $fr -gt $((fb + 60)) ]; then echo amber
    elif [ $fr = $fg ] && [ $fg = $fb ] && [ $fr -lt 230 ]; then echo grey
    else echo other; fi
}
start
[ "$(kind)" = white-on-blue ] || fail "first start: not white on dark blue ($(colours))"
blue=$(colours)
at 2 10                                               # GREEN (SCREEN on row 9, the four below)
[ "$(kind)" = green ] || fail "GREEN ($(colours))"
at 8 10                                               # AMBER
[ "$(kind)" = amber ] || fail "AMBER ($(colours))"
amber=$(colours)
at 9 9                                                # TOGGLE: to blue
[ "$(colours)" = "$blue" ] || fail "TOGGLE to blue ($(colours))"
at 9 9                                                # TOGGLE: back to amber
[ "$(colours)" = "$amber" ] || fail "TOGGLE back to amber ($(colours))"
at 14 10                                              # GREY
[ "$(kind)" = grey ] || fail "GREY ($(colours))"
grey=$(colours)
cat ~/personalities/pet/palette
grep -q "^palette=grey" ~/personalities/pet/palette || fail "the choice not kept"
kill $PID; sleep 3
start                                                 # the next start: grey, as chosen
[ "$(colours)" = "$grey" ] || fail "the next start is not grey ($(colours))"
at 19 10                                              # BLUE
[ "$(colours)" = "$blue" ] || fail "BLUE ($(colours))"
grep "k4510c64: the PET" ~/log
echo "test-petscreen: OK (white on blue first, GREEN AMBER GREY BLUE and TOGGLE change it, kept for the next start; blue $blue, amber $amber, grey $grey)"'
