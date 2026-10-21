#!/bin/sh
# The Apple ][ personality (AppleWin's sa2 and its sidebar, Doc 2026-10-20)
# under Xvfb, checked in its log -- no pictures: the sidebar beside the
# picture; SCREEN GREEN, TOGGLE-INVERSE, BLUE and COLOR; F12 OPTIONS (by
# its keys and the mouse); MODEL //E (the machine started again); a disk
# from the list in drive 1; QUIT; and the next start keeps the model, the
# screen, inverse and the disk.
#   tools/test-apple2.sh      (after tools/make-images.sh apple2; needs k4510-base-xdo)
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd); W=$HERE/work
rm -rf $W/test-apple2 $W/test-a2home
unsquashfs -q -d $W/test-apple2 $W/out/personalities/apple2.squashfs
mkdir -p $W/test-a2home/personalities
cp -r $W/out/home/personalities/apple2 $W/test-a2home/personalities/
podman run --rm -v $W/test-apple2:/opt/personalities/apple2:ro,Z -v $W/test-a2home:/home/k4510:Z \
    -e HOME=/home/k4510 -e SDL_AUDIODRIVER=dummy -e K4510_PLACEMENT=left k4510-base-xdo sh -c '
A=/opt/personalities/apple2
Xvfb :1 -screen 0 1920x1080x24 >/dev/null 2>&1 & sleep 1; export DISPLAY=:1
fail() { cat ~/log; echo "test-apple2: FAILED: $1"; exit 1; }
start() {
    stdbuf -oL $A/run/apple2 --no-audio >~/log 2>&1 & PID=$!
    for i in $(seq 20); do w=$(xdotool search --name "Apple" 2>/dev/null | head -1); [ -n "$w" ] && break; sleep 0.5; done
    [ -n "$w" ] || fail "no window"
    xdotool windowmove $w 0 0 windowsize $w 1920 1080; sleep 3
    set -- $(sed -n "s/.*k4510apple2: the panel at \([0-9]*\),\([0-9]*\), \([0-9]*\)x\([0-9]*\) machine pixels x\([0-9]*\).*/\1 \2 \3 \4 \5/p" ~/log | tail -1)
    [ -n "$5" ] || fail "no panel"
    X=$1 Y=$2 K=$5 PH=$4
    set -- $(sed -n "s/.*options at \([0-9]*\),\([0-9]*\) x\([0-9]*\).*/\1 \2 \3/p" ~/log | tail -1)
    [ -n "$3" ] || fail "no options box"
    MX=$1 MY=$2 MS=$3
    xdotool windowfocus $w 2>/dev/null || true
}
key() { xdotool key $1; sleep 0.5; }
box() { xdotool mousemove $((MX + ($1 * 8 + 4) * MS)) $((MY + ($2 * 8 + 4) * MS)); sleep 0.3; xdotool click 1; sleep 1; }
at() { xdotool mousemove $((X + ($1 * 8 + 4) * K)) $((Y + ($2 * 8 + 4) * K)); sleep 0.3; xdotool click 1; sleep ${3:-1}; }
start
grep "k4510apple2: the panel" ~/log | tail -1
[ $K -ge 2 ] || fail "the panel at x1, too small to read"
grep "k4510apple2: the panel" ~/log | tail -1 | grep -q "model iieenh, screen color" || fail "not the enhanced //e in colour at first"
grep -q "k4510apple2: drive 1 turning" ~/log || fail "drive 1 never turned: DOS 3.3 did not boot"
at 7 13                                               # GREEN (SCREEN on row 12, the four below)
grep -q "k4510apple2: the screen: green" ~/log || fail "GREEN"
[ "$(cat ~/personalities/apple2/screen)" = green ] || fail "the screen not kept"
at 9 12                                               # TOGGLE-INVERSE: on
grep -q "k4510apple2: inverse: on" ~/log || fail "TOGGLE-INVERSE"
[ "$(cat ~/personalities/apple2/inverse)" = on ] || fail "inverse not kept"
at 26 13                                              # BLUE, the fifth
grep -q "k4510apple2: the screen: blue" ~/log || fail "BLUE"
at 2 13                                               # COLOR
grep -q "k4510apple2: the screen: color" ~/log || fail "COLOR"
key F12                                               # OPTIONS
grep -q "k4510apple2: options open" ~/log || fail "F12 did not open OPTIONS"
key Left                                              # SCANLINES: strong to soft
grep -q "k4510apple2: scanlines: soft" ~/log || fail "OPTIONS SCANLINES"
key Down; key Down; key Right                         # DISK: fast to real
grep -q "k4510apple2: disk: real" ~/log || fail "OPTIONS DISK"
box 19 4                                              # PIXELS SOFT, by the mouse (row 4, from col 19)
grep -q "k4510apple2: pixels: soft" ~/log || fail "OPTIONS PIXELS by the mouse"
key Escape
grep -q "k4510apple2: options closed" ~/log || fail "Esc did not close OPTIONS"
for f in scanlines:soft disk:real pixels:soft; do [ "$(cat ~/personalities/apple2/${f%%:*})" = ${f#*:} ] || fail "$f not kept"; done
kill -0 $PID 2>/dev/null || fail "sa2 died in OPTIONS"
at 8 16 3                                             # MODEL: //E (row 15, the four below)
grep -q "k4510apple2: the model: iie$" ~/log || fail "MODEL //E"
kill -0 $PID 2>/dev/null || fail "sa2 died after the model"
at 2 20 2                                             # the disks (header on row 18): PRODOS-2.4.3.PO, the second
grep -q "k4510apple2: drive 1: /home/k4510/personalities/apple2/PRODOS-2.4.3.PO$" ~/log || fail "the disk did not go in"
n1=$(grep -c "k4510apple2: drive 1 turning" ~/log); sleep 3
[ $(grep -c "k4510apple2: drive 1 turning" ~/log) -gt 0 ] || fail "ProDOS did not boot"
at 2 25 2                                             # nothing there: nothing happens
kill -0 $PID 2>/dev/null || fail "sa2 died"
grep "k4510apple2" ~/log
rows=$((PH / 8))
n=9; [ $n -gt $((rows - 18 - 10)) ] && n=$((rows - 18 - 10))
at 18 $((rows - n - 3)) 2                             # QUIT: after EJECT and RESET
grep -q "k4510apple2: quit" ~/log || fail "QUIT"
sleep 2; kill -0 $PID 2>/dev/null && fail "sa2 still running after QUIT"
start                                                 # the next start: the //e, colour, ProDOS
grep "k4510apple2: the panel" ~/log | tail -1
grep "k4510apple2: the panel" ~/log | tail -1 | grep -q "model iie, screen color, inverse on" || fail "the next start: not the //e in inverse colour"
grep -q "PRODOS-2.4.3.PO" /proc/$PID/cmdline || fail "the next start: not ProDOS in drive 1"
kill $PID
echo "test-apple2: OK (the sidebar, SCREEN GREEN, TOGGLE-INVERSE, BLUE, COLOR, F12 OPTIONS, MODEL //E, a disk in drive 1, QUIT, all kept for the next start)"'
