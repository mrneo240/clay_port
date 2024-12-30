#!/bin/sh
psp-fixup-imports $1
psp-prxgen $1 `basename $1`.prx
mksfo  'Clayculator' PARAM.SFO

ICON="NULL"
ICON_FILE="ICON0.png"
if [ -f "$ICON_FILE" ]; then
  ICON="ICON0.png"
fi

pack-pbp EBOOT.PBP PARAM.SFO "$ICON" NULL NULL NULL NULL `basename $1`.prx NULL
cp `basename $1`.prx EBOOT.BIN
cp `basename $1`.prx BOOT.BIN

#cp `basename $1`.prx ../
#cp `basename $1` ../wipeout-rewrite_psp.elf
