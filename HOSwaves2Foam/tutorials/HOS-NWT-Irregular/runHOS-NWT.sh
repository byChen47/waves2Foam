#!/bin/bash
#
# Run the irregular-wave HOS-NWT case and produce
# Results/modes_HOS_SWENSE.dat for the waves2Foam Grid2Grid coupling.
#
# Usage:
#   bash runHOS-NWT.sh [/path/to/HOS_NWT]
#
# If no path is given, the executable is looked up in
# $WAVES_DIR/HOSwaves2Foam/src/HOS-NWT/build/HOS_NWT (or $HOSNWT_BIN).

set -e

HOSNWT_BIN=${1:-$HOSNWT_BIN}
if [ -z "$HOSNWT_BIN" ] && [ -n "$WAVES_DIR" ] && [ -x "$WAVES_DIR/HOSwaves2Foam/src/HOS-NWT/build/HOS_NWT" ]
then
    HOSNWT_BIN="$WAVES_DIR/HOSwaves2Foam/src/HOS-NWT/build/HOS_NWT"
fi
if [ -z "$HOSNWT_BIN" ] || [ ! -x "$HOSNWT_BIN" ]
then
    echo "ERROR: HOS-NWT executable not found."
    echo "Build it with HOSwaves2Foam/scripts/buildHOS.sh"
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
