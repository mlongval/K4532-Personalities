#!/bin/sh
# Scalos 41.7 (the Scalos Team's last 68k beta, 2010; GPL/LGPL) -- the
# Workbench replacement the A1200 can start instead of Workbench 3.1 (the
# sidebar's Desktop, F11) -- and PeterK's icon.library 46.4 (its v51 TC020
# build, with Thomas Rapp's RemLib; freeware, Aminet util/libs), which the
# NewIcons and colour icons both desktops show need.  scalos.noname.fr is
# gone, so Scalos comes from the Internet Archive's copy; its readme asks it
# be had only from the Scalos site, so it is fetched, never kept in this
# repository.  And MUI 3.8 (Stefan Stuntz; shareware, works unregistered)
# with the custom classes Scalos's Prefs and modules use (NList,
# BetterString, TextEditor, Lamp, Urltext, Textinput, Popplaceholder from
# Aminet; Pophotkey, Voyager's, from the Internet Archive's copy of its site).  Into work/scalos; make-images.sh lays both out on a DH0 tree
# in the image and the A1200's run script copies that onto DH0 once.
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd); D=$HERE/work/scalos
mkdir -p "$D"
curl -fsSL -o "$D/ScalosBeta_41.7_68K.lha" \
    'https://web.archive.org/web/20201028163940id_/http://scalos.noname.fr/products/scalos/main/ScalosBeta_41.7_68K.lha'
curl -fsSL -o "$D/IconLib_46.4.lha" https://aminet.net/util/libs/IconLib_46.4.lha
mkdir -p "$D/mui"
curl -fsSL -o "$D/mui/mui38usr.lha" https://aminet.net/util/libs/mui38usr.lha
for m in MCC_NList-0.128 MCC_BetterString-11.36 MCC_TextEditor-15.56 MCC_Lamp MCC_Urltext MCC_TextInput MCC_Popph; do
    curl -fsSL -o "$D/mui/$m.lha" "https://aminet.net/dev/mui/$m.lha"
done
curl -fsSL -o "$D/mui/pophotkey_1715.lha" \
    'https://web.archive.org/web/20061230235225id_/http://vapor.meanmachine.ch:80/voyager/pophotkey_1715.lha'
# ilbm.datatype 44 (24-bit ILBMs), for Scalos's desktop picture (Aminet)
mkdir -p "$D/dt"
for p in ilbmdt44; do curl -fsSL -o "$D/dt/$p.lha" "https://aminet.net/util/dtype/$p.lha"; done
ls -l "$D" "$D/mui" "$D/dt"
