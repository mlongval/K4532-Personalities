#!/bin/sh
# AppleWin's Linux SDL2 frontend (sa2, github.com/audetto/AppleWin, GPL) for
# the Apple ][ personality.  Runs INSIDE the builder.  The ROMs ( ][, ][+,
# //e, the Disk II's) are AppleWin's own, compiled into the binary; run/apple2
# starts it with --no-imgui (the plain SDL renderer, as VICE's) and the
# sidebar (tools/patches/apple2).
set -e
REV=${APPLEWIN_REV:-a1e20c0362f392dc25f1a3b218d2f84d4f1a25da}   # 2026-10-10
cd /work
[ -d applewin-src/.git ] || git clone -q https://github.com/audetto/AppleWin.git applewin-src
cd applewin-src
git cat-file -e "$REV^{commit}" 2>/dev/null || git fetch -q origin
git checkout -qf "$REV"
git reset -q --hard && git clean -qfdx
git submodule update -q --init source/frontends/sdl/imgui/imgui source/frontends/sdl/imgui/imgui_club \
    source/frontends/sdl/imgui/imgui-filebrowser
python3 /tools/patches/apple2/apply.py
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SA2=ON -DENABLE_NETWORKING=OFF \
    -DCMAKE_CXX_FLAGS="-march=x86-64-v2" >/work/apple2-cmake.log 2>&1 || { tail -20 /work/apple2-cmake.log; exit 1; }
ninja -C build sa2 >/work/apple2-make.log 2>&1 || { grep -m10 -A3 "error" /work/apple2-make.log; exit 1; }
rm -rf /work/stage/apple2 && mkdir -p /work/stage/apple2/bin /work/stage/apple2/disks
cp build/sa2 /work/stage/apple2/bin/
# the first disks: DOS 3.3's System Master and ProDOS 2.4.3, as AppleWin ships them
cp "bin/DOS 3.3 System Master - 680-0210-A.dsk" /work/stage/apple2/disks/DOS33-MASTER.DSK
cp bin/ProDOS_2_4_3.po /work/stage/apple2/disks/PRODOS-2.4.3.PO
ls -l /work/stage/apple2/bin /work/stage/apple2/disks
