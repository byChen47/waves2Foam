#!/bin/bash
#
# Build a 3D-capable HOS-NWT executable (y-direction modes n2 > 1).
#
# The vendored HOS-NWT source keeps n2 = 1 for the 2D tutorials; this script
# temporarily patches `sources/variabledef/common_vars.f90`, builds a
# separate binary into src/HOS-NWT/build3D, and restores the source file
# afterwards (even on failure).
#
# Usage:
#   bash HOSwaves2Foam/scripts/buildHOS3D.sh [N2] [N1]
#
# N2: number of y-modes (default 8); N1: number of x-modes (default 128).
# Smaller N1 keeps the SWENSE output file size reasonable
# (size scales with n1*n2).  Output:
#   HOSwaves2Foam/src/HOS-NWT/build3D/HOS_NWT

set -e

N2=${1:-8}
N1=${2:-128}

MODULE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
THIRD="$MODULE_ROOT/ThirdParty"
FFTW_ROOT="$THIRD/fftw3"
LAPACK_DIR="$THIRD/lapack"

SRC="$MODULE_ROOT/src/HOS-NWT"
CFG="$SRC/sources/variabledef/common_vars.f90"
BUILD="$SRC/build3D"

command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake not found"; exit 1; }
command -v gfortran >/dev/null 2>&1 || { echo "ERROR: gfortran not found"; exit 1; }
[ -f "$CFG" ] || { echo "ERROR: $CFG not found"; exit 1; }

# Patch n2 -> N2 and restore on exit
cp "$CFG" "$CFG.hos3d.bak"
restore() { mv -f "$CFG.hos3d.bak" "$CFG"; }
trap restore EXIT
sed -i "s/^INTEGER, PARAMETER :: n2 = .*/INTEGER, PARAMETER :: n2 = $N2/" "$CFG"
sed -i "s/^INTEGER, PARAMETER :: n1 = .*/INTEGER, PARAMETER :: n1 = $N1/" "$CFG"
grep -n "PARAMETER :: n2" "$CFG" | head -1
grep -n "PARAMETER :: n1" "$CFG" | head -1

rm -rf "$BUILD"
cmake -H"$SRC/cmake" -B"$BUILD" \
      -DCMAKE_BUILD_TYPE=Release \
      -DFFTW_ROOT="$FFTW_ROOT" \
      -DCMAKE_LIBRARY_PATH="$LAPACK_DIR" \
      -DCMAKE_EXE_LINKER_FLAGS="-L$LAPACK_DIR"
mkdir -p "$BUILD/lib"
cmake --build "$BUILD" -j1

echo "OK: 3D HOS-NWT (n1=$N1, n2=$N2) -> $BUILD/HOS_NWT"
