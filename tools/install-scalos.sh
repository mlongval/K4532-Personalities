#!/bin/sh
# Scalos 41.7 and PeterK's icon.library onto an A1200 DH0 tree ($1), as
# Scalos's installer would put it there (its Install3.5_ScalosBeta read and
# done on the host): SYS:Scalos (Scalos: is assigned to it), its libraries
# and icon datatypes, the default prefs in ENVARC:Scalos, and its LoadWB as
# C:LoadWB.scalos beside Workbench's own (the Startup-Sequence picks one by
# S:Desktop-Scalos); icon.library v51 in Libs: with RemLib in C:; and
# MUI 3.8 in SYS:MUI as its installer puts it (its User-Startup lines are
# run/amiga's), the custom classes in MUI:Libs/mui.  Needs the archives of
# tools/get-scalos.sh in $2 (work/scalos).
set -e
H=$1; Z=$(cd "${2:-.}" && pwd)
[ -n "$H" ] && [ -f "$Z/ScalosBeta_41.7_68K.lha" ] && [ -f "$Z/IconLib_46.4.lha" ] || { echo "usage: $0 DH0 work/scalos"; exit 1; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
(cd $t && lha xq "$Z/ScalosBeta_41.7_68K.lha" && lha xq "$Z/IconLib_46.4.lha")
X=$t/ScalosBeta; I=$t/IconLib_46.4
S=$H/Scalos
mkdir -p "$H/C" "$H/Libs" "$S/Modules" "$S/Prefs" "$S/Plugins/FileTypes" "$S/Plugins/Menu" "$S/Plugins/OOP" "$S/Plugins/Preview" "$S/Tools" "$S/Themes"
cp "$X/Scalos" "$X/Scalos.info" "$X/History" "$X/readme.txt" "$S/"
cp -r "$X/modules/." "$S/Modules/"
cp -r "$X/Prefs/." "$S/Prefs/"
for p in FileTypes Menu OOP Preview; do cp -r "$X/Plugins/$p/." "$S/Plugins/$p/"; done
cp -r "$X/Tools/." "$S/Tools/"
cp -r "$X/Themes/default" "$S/Themes/"
for l in iconobject sqlite3 popupmenu preferences scalosgfx guigfx render; do cp "$X/libs/$l.library" "$H/Libs/"; done
mkdir -p "$H/Classes/DataTypes" "$H/Devs/DataTypes"
cp "$X"/Classes/Datatypes/*.datatype "$H/Classes/DataTypes/"
cp "$X"/Devs/Datatypes/* "$H/Devs/DataTypes/"
E=$H/Prefs/Env-Archive; F=$X/env-archive/scalos.68K; mkdir -p "$E/Scalos/FileTypes"
cp -r "$F/icandy" "$F/Palette13.prefs" "$F/Pattern.prefs" "$F/Persist.prefs" "$F/scalos.prefs" "$F/English/Menu13.prefs" "$E/Scalos/"
cp -r "$F/English/FileTypes/." "$E/Scalos/FileTypes/"
cp "$X/env-archive/deficons.prefs" "$E/"
cp "$X/c/LoadWB" "$H/C/LoadWB.scalos"
# the Workbench screen uaegfx's 640x480 in HiColor (Doc, 2026-10-18: "a
# 640x480 16 bit color fancy scalos screen"), not Workbench 3.1's 4 colours,
# too few for Scalos's palette and the colour icons.  HiColor is 15-bit
# here (R5G5B5, 32768 colours): Picasso96's uaegfx settings have no 16-bit
# modes, and Picasso96 numbers a resolution's modes by depth -- +0x000
# 8-bit, +0x100 15-bit, +0x302 32-bit (read from Amiberry's log: 640x480
# is 0x50031000)
mkdir -p "$E/Sys"; python3 "$(dirname "$0")/amiga-screenmode.py" "$E/Sys/screenmode.prefs" 15 0x50031100 640 480
# the desktop picture: ilbm.datatype 44 to read it in 24 bits (shown by
# Picasso96's own picture.datatype, which make-images.sh puts beside it --
# Aminet's picture.datatype 43, made for CyberGraphX, stopped Scalos from
# starting at all), and the theme's own Background (a JPEG, which 3.1
# cannot read) as a 24-bit ILBM of the screen's size
d=$t/dt; mkdir -p "$d" && (cd "$d" && lha xq "$Z/dt/ilbmdt44.lha")
cp "$d/ilbmdt44/ilbm.datatype" "$H/Classes/DataTypes/ilbm.datatype"
python3 "$(dirname "$0")/amiga-ilbm.py" "$X/Themes/default/Desktop/Background" "$S/Themes/default/Desktop/Background" 640 480
cp "$I/Libs/icon.library" "$H/Libs/icon.library"
cp "$I/ThirdParty/RemLib/RemLib" "$H/C/RemLib"
# MUI: the package's drawer less its installer, and the 68k classes
M=$H/MUI; m=$t/mui; mkdir -p "$m" && (cd "$m" && lha xq "$Z/mui/mui38usr.lha")
cp -r "$m/MUI" "$H/"; cp "$m/MUI.info" "$H/" 2>/dev/null || true
rm -rf "$M/Install-MUI" "$M/Install-MUI.info" "$M/Installer433"
for a in "$Z"/mui/MCC_*.lha; do (cd "$m" && lha xq "$a"); done
for c in BetterString.mcc BetterString.mcp HotkeyString.mcc NBalance.mcc NBitmap.mcc NFloattext.mcc NList.mcc \
         NListtree.mcc NListtree.mcp NListview.mcc NListviews.mcp TextEditor.mcc TextEditor.mcp; do
    cp "$(find "$m" -path '*AmigaOS3*' -name "$c" | head -1)" "$M/Libs/mui/$c"
done
cp "$m/MCC_Lamp/Libs/MUI/Lamp020.mcc" "$M/Libs/mui/Lamp.mcc"; cp "$m/MCC_Lamp/Libs/MUI/Lamp.mcp" "$M/Libs/mui/"
cp "$m/MCC_Urltext/MUI/Urltext.mcc" "$m/MCC_Urltext/MUI/Urltext.mcp" "$M/Libs/mui/"
cp "$m/MCC_Textinput/MUI/Textinput.mcc" "$m/MCC_Textinput/MUI/Textinput.mcp" "$m/MCC_Textinput/MUI/Textinputscroll.mcc" "$M/Libs/mui/"
cp "$m/MCC_Popph/Libs/mui/Popplaceholder.mcc" "$M/Libs/mui/"
(cd "$M/Libs/mui" && lha xq "$Z/mui/pophotkey_1715.lha")
chmod -R a+rX,u+w "$H"
chmod a+x "$S/Scalos" "$H/C/LoadWB.scalos" "$H/C/RemLib" "$H"/Libs/*.library "$M"/Libs/*.library "$M"/Libs/mui/*   # Amiberry gives AmigaDOS the e bit from it
find "$S/Prefs" "$S/Tools" "$M" -type f ! -name '*.info' -exec chmod a+x {} +
