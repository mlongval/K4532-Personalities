#!/bin/sh
# The A1200's two desktops (Doc, 2026-10-18), under Xvfb, read from logs and
# files -- no pictures:
#   the run script puts Scalos and icon.library 51 on a DH0 that has neither
#   and teaches its Startup-Sequence both; Workbench 3.1 comes up (no
#   scalos.library in memory); F11 (the sidebar's Desktop) makes
#   S:Desktop-Scalos, and the next start runs Scalos (scalos.library 41.7 in
#   memory), and MUI works: Scalos Prefs (a MUI program, NList and the
#   other classes) opens its ARexx port; F11 again takes it away.  Version
#   and ARexx's show('P'), run at the end of the Startup-Sequence (lines this
#   test adds, S:test-tail), write what they find to SYS:ver and SYS:ports.
#   tools/test-amiga-desktop.sh      (after tools/make-images.sh amiga; needs k4510-base-xdo)
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd); W=$HERE/work
rm -rf $W/test-amiga $W/test-ahome
unsquashfs -q -d $W/test-amiga $W/out/personalities/amiga.squashfs
mkdir -p $W/test-ahome/personalities && cp -a $W/out/home/personalities/a1200 $W/test-ahome/personalities/
rm -f $W/test-ahome/personalities/a1200/a1200.uae
cat > $W/test-ahome/test-tail <<'TAIL'
Wait 25
C:Version scalos.library >SYS:ver
C:Version icon.library >>SYS:ver
C:Version muimaster.library >>SYS:ver
IF EXISTS S:Desktop-Scalos
  Run >NIL: SYS:System/RexxMast
  Run >NIL: "Scalos:Prefs/Scalos Prefs"
  Wait 40
  SYS:Rexxc/rx "call open(f,'SYS:ports','W'); call writeln(f,show('P')); call close(f)"
EndIF
TAIL
podman run --rm -v $W/test-amiga:/opt/personalities/amiga:ro,Z -v $W/test-ahome:/home/k4510:Z \
    -e HOME=/home/k4510 -e SDL_AUDIODRIVER=dummy -e K4510_PLACEMENT=left k4510-base-xdo sh -c '
    Xvfb :1 -screen 0 1920x1080x24 >/dev/null 2>&1 & sleep 1; export DISPLAY=:1
    H=~/personalities/a1200/DH0
    fail() { tail -30 ~/a.log; echo "test-amiga-desktop: FAILED: $1"; exit 1; }
    boot() {   # seconds: start, wait, stop; SYS:ver says which desktop ran
        rm -f $H/ver
        /opt/personalities/amiga/run/a1200 --log >~/a.log 2>&1 & P=$!; sleep $1
        kill $P 2>/dev/null; sleep 2
    }
    f11() {
        /opt/personalities/amiga/run/a1200 --log >~/a.log 2>&1 & P=$!; sleep 12
        w=$(xdotool search --name . | tail -1); xdotool windowfocus $w 2>/dev/null; sleep 0.5; xdotool key F11; sleep 2
        kill $P 2>/dev/null; sleep 2
    }
    boot 8                                            # the first start: the run script installs
    [ -f $H/C/LoadWB.scalos ] && [ -f $H/Scalos/Scalos ] && [ -f $H/Libs/icon.library ] || fail "Scalos not copied onto DH0"
    grep -q "RemLib >NIL: icon.library" $H/S/Startup-Sequence || fail "no RemLib in the Startup-Sequence"
    grep -q "IF EXISTS S:Desktop-Scalos" $H/S/Startup-Sequence || fail "the Startup-Sequence does not choose"
    grep -q "BEGIN MUI" $H/S/User-Startup && [ -f $H/MUI/Libs/muimaster.library ] && [ -f $H/MUI/Libs/mui/NList.mcc ] || fail "MUI not installed"
    cp ~/test-tail $H/S/test-tail
    sed -i "s|^EndCLI >NIL:\$|Execute S:test-tail\nEndCLI >NIL:|" $H/S/Startup-Sequence
    boot 60
    cat $H/ver; grep -q "icon.library 51" $H/ver || fail "icon.library 51 not in use"
    grep -q "^scalos.library" $H/ver && fail "Scalos ran, Workbench was asked for"
    f11
    grep -a "k4510amiga: desktop" ~/a.log; [ -f $H/S/Desktop-Scalos ] || fail "F11 did not choose Scalos"
    boot 160; grep -a -i -E "bitplane|planes|depth|AGA|lores|hires|screen mode|shres|BPLCON" ~/a.log | sort | uniq -c | sort -rn | head -12
    cat $H/ver; grep -q "^scalos.library 41" $H/ver || fail "Scalos did not start"
    grep -q "^muimaster.library 19" $H/ver || fail "no muimaster.library (MUI: not assigned?)"
    cat $H/ports 2>/dev/null; grep -q "PREFERENCES" $H/ports 2>/dev/null || fail "Scalos Prefs (MUI) opened no ARexx port (a class missing?)"
    f11
    grep -a "k4510amiga: desktop" ~/a.log; [ ! -f $H/S/Desktop-Scalos ] || fail "F11 again did not choose Workbench"
    echo "test-amiga-desktop: OK (Scalos and MUI installed once; Workbench, then F11, Scalos 41 with Scalos Prefs running on MUI, then F11, Workbench)"'
