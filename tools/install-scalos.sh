#!/bin/sh
# Scalos 41.7 and PeterK's icon.library onto an A1200 DH0 tree ($1), as
# Scalos's installer would put it there (its Install3.5_ScalosBeta read and
# done on the host): SYS:Scalos (Scalos: is assigned to it), its libraries
# and icon datatypes, the default prefs in ENVARC:Scalos, and its LoadWB as
# C:LoadWB.scalos beside Workbench's own (the Startup-Sequence picks one by
# S:Desktop-Scalos); icon.library v51 in Libs: with RemLib in C:.  Needs
# the archives of tools/get-scalos.sh in $2 (work/scalos).
set -e
H=$1; Z=$2
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
cp "$I/Libs/icon.library" "$H/Libs/icon.library"
cp "$I/ThirdParty/RemLib/RemLib" "$H/C/RemLib"
chmod -R a+rX,u+w "$H"
chmod a+x "$S/Scalos" "$H/C/LoadWB.scalos" "$H/C/RemLib" "$H"/Libs/*.library   # Amiberry gives AmigaDOS the e bit from it
