#!/bin/bash
#
# Build HOS-NWT and HOS-ocean using only the dependencies vendored in
# HOSwaves2Foam/ThirdParty (FFTW3 static + LAPACK/BLAS static).
# No system package installs are required besides cmake and gfortran.
#
# Usage:
#   bash HOSwaves2Foam/scripts/buildHOS.sh
#
# Outputs:
#   HOSwaves2Foam/src/HOS-NWT/build/HOS_NWT
#   HOSwaves2Foam/src/HOS-ocean/build/HOS_ocean
#
# If the executables already exist, they are kept (pass --force to rebuild).

set -e

FORCE=0
[ "${1:-}" = "--force" ] && FORCE=1

MODULE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
THIRD="$MODULE_ROOT/ThirdParty"
FFTW_ROOT="$THIRD/fftw3"
LAPACK_DIR="$THIRD/lapack"

# --- prerequisites ---------------------------------------------------------
command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake not found (apt install cmake)"; exit 1; }
command -v gfortran >/dev/null 2>&1 || { echo "ERROR: gfortran not found (apt install gfortran)"; exit 1; }

for f in "$FFTW_ROOT/libfftw3.a" "$FFTW_ROOT/fftw3.f03" \
         "$LAPACK_DIR/liblapack.a" "$LAPACK_DIR/libblas.a"
do
    [ -f "$f" ] || { echo "ERROR: vendored dependency missing: $f"; exit 1; }
done

# --- build one model -------------------------------------------------------
build_model () {
    name="$1"
    bin="$2"
    src="$MODULE_ROOT/src/$name"

    if [ ! -f "$src/cmake/CMakeLists.txt" ]; then
        echo "SKIP: $name (cmake project not found)"
        return
    fi

    if [ "$FORCE" -eq 0 ] && [ -x "$src/build/$bin" ]
    then
        echo "SKIP: $name already built ($src/build/$bin); use --force to rebuild"
        return
    fi

    echo "============================================================="
    echo " Building $name"
    echo "============================================================="

    rm -rf "$src/build"
    cmake -H"$src/cmake" -B"$src/build" \
        -DCMAKE_BUILD_TYPE=Release \
        -DFFTW_ROOT="$FFTW_ROOT" \
        -DCMAKE_LIBRARY_PATH="$LAPACK_DIR" \
        -DCMAKE_EXE_LINKER_FLAGS="-L$LAPACK_DIR"

    # gfortran writes module files into build/lib; the directory must exist
    # and the build must run single-threaded to avoid .mod0 rename races.
    mkdir -p "$src/build/lib"
    cmake --build "$src/build" -j1
}

build_model HOS-NWT HOS_NWT
build_model HOS-ocean HOS_ocean

echo
echo "Build finished. Executables:"
ls -l "$MODULE_ROOT/src/HOS-NWT/build/HOS_NWT" 2>/dev/null || echo "  (HOS-NWT not built)"
ls -l "$MODULE_ROOT/src/HOS-ocean/build/HOS_ocean" 2>/dev/null || echo "  (HOS-ocean not built)"
