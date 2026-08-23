#!/bin/bash
#
# Run the 3D HOS-NWT case (n1=128, n2=8) and produce
# Results/modes_HOS_SWENSE.dat for the waves2Foam Grid2Grid coupling.
#
# Usage:
#   bash runHOS-NWT.sh /path/to/HOS_NWT_executable
#
# If no path is given, the 3D executable is looked up in
# $WAVES_DIR/HOSwaves2Foam/src/HOS-NWT/build3D/HOS_NWT (built with
# HOSwaves2Foam/scripts/buildHOS3D.sh), or as $HOSNWT_BIN.

set -e

HOSNWT_BIN=${1:-$HOSNWT_BIN}
if [ -z "$HOSNWT_BIN" ] && [ -n "$WAVES_DIR" ] && [ -x "$WAVES_DIR/HOSwaves2Foam/src/HOS-NWT/build3D/HOS_NWT" ]
then
    HOSNWT_BIN="$WAVES_DIR/HOSwaves2Foam/src/HOS-NWT/build3D/HOS_NWT"
fi
if [ -z "$HOSNWT_BIN" ] || [ ! -x "$HOSNWT_BIN" ]
then
    echo "ERROR: HOS-NWT executable not found."
    echo "Build the 3D executable with HOSwaves2Foam/scripts/buildHOS3D.sh"
    exit 1
fi

mkdir -p Results
"$HOSNWT_BIN"

if [ -f Results/modes_HOS_SWENSE.dat ]
then
    echo "OK: Results/modes_HOS_SWENSE.dat written"
else
    echo "ERROR: Results/modes_HOS_SWENSE.dat not found"
    exit 1
fi
