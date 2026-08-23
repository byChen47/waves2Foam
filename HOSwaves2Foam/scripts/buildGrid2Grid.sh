#!/bin/bash
#
# Build and install libGrid2Grid.so (LHEEA Grid2Grid HOS wrapper) into
# $FOAM_USER_LIBBIN, as required by libwaves2FoamGrid2Grid.so.
#
# Usage:
#   bash HOSwaves2Foam/scripts/buildGrid2Grid.sh [--force]
#
# If libGrid2Grid.so is already installed, it is kept (pass --force to
# rebuild).
#
# Requirements: cmake, gfortran, FFTW3 (libfftw3-dev).

set -e

FORCE=0
[ "${1:-}" = "--force" ] && FORCE=1

if [ -z "$FOAM_USER_LIBBIN" ]
then
    echo "ERROR: not in an OpenFOAM environment (FOAM_USER_LIBBIN is empty)."
    exit 1
fi

G2G_URL=${G2G_URL:-https://github.com/LHEEA/Grid2Grid.git}
G2G_SRC=${G2G_SRC:-/tmp/Grid2Grid}
G2G_BUILD="$G2G_SRC/build"

# Use the FFTW3 vendored in HOSwaves2Foam/ThirdParty (no system install needed)
MODULE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FFTW_ROOT="$MODULE_ROOT/ThirdParty/fftw3"

if [ ! -d "$G2G_SRC/.git" ]
then
    echo "Cloning Grid2Grid from $G2G_URL"
    git clone --depth 1 "$G2G_URL" "$G2G_SRC"
fi

if [ "$FORCE" -eq 0 ] && [ -f "$FOAM_USER_LIBBIN/libGrid2Grid.so" ]
then
    echo "SKIP: libGrid2Grid.so already installed in $FOAM_USER_LIBBIN"
    echo "      (use --force to rebuild)"
    exit 0
fi

echo "Configuring Grid2Grid (BUILD_OF_LIB=ON, output: $FOAM_USER_LIBBIN)"
cmake -H"$G2G_SRC" -B"$G2G_BUILD" -DBUILD_OF_LIB=ON \
      -DFFTW_ROOT="$FFTW_ROOT"

# The Fortran module files are written to $G2G_BUILD/lib; make sure it exists
# and build single-threaded to avoid gfortran .mod0 rename races.
mkdir -p "$G2G_BUILD/lib"
cmake --build "$G2G_BUILD" -j1

if [ -f "$FOAM_USER_LIBBIN/libGrid2Grid.so" ]
then
    echo "OK: libGrid2Grid.so installed in $FOAM_USER_LIBBIN"
else
    echo "ERROR: libGrid2Grid.so was not installed."
    exit 1
fi
