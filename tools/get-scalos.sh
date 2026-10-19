#!/bin/sh
# Scalos 41.7 (the Scalos Team's last 68k beta, 2010; GPL/LGPL) -- the
# Workbench replacement the A1200 can start instead of Workbench 3.1 (the
# sidebar's Desktop, F11) -- and PeterK's icon.library 46.4 (its v51 TC020
# build, with Thomas Rapp's RemLib; freeware, Aminet util/libs), which the
# NewIcons and colour icons both desktops show need.  scalos.noname.fr is
# gone, so Scalos comes from the Internet Archive's copy; its readme asks it
# be had only from the Scalos site, so it is fetched, never kept in this
# repository.  Into work/scalos; make-images.sh lays both out on a DH0 tree
# in the image and the A1200's run script copies that onto DH0 once.
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd); D=$HERE/work/scalos
mkdir -p "$D"
curl -fsSL -o "$D/ScalosBeta_41.7_68K.lha" \
    'https://web.archive.org/web/20201028163940id_/http://scalos.noname.fr/products/scalos/main/ScalosBeta_41.7_68K.lha'
curl -fsSL -o "$D/IconLib_46.4.lha" https://aminet.net/util/libs/IconLib_46.4.lha
ls -l "$D"
