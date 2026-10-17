#!/bin/sh
# Picasso96 2.0 (Tobias Abt, Alexander Kneer; Aminet driver/video/Picasso96.lha):
# the Amiga's RTG system, with uaegfx.card, the driver for UAE's (Amiberry's)
# graphics card.  Into work/picasso96; make-images.sh lays it out as an
# install (the installer's own steps, done on the host) and the A1200's run
# script copies it onto DH0 once.  Shareware ("no distribution restrictions
# as long as the archive is complete"; the authors ask US$20 of a user):
# fetched from Aminet, never kept in this repository.
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd); D=$HERE/work/picasso96
mkdir -p "$D"
curl -fsSL -o "$D/Picasso96.lha" https://aminet.net/driver/video/Picasso96.lha
curl -fsSL -o "$D/Picasso96.readme" https://aminet.net/driver/video/Picasso96.readme
ls -l "$D"
