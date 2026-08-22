# waves2Foam

A toolbox for **volume-of-fluid (VOF) based wave generation and active wave absorption** in OpenFOAM, based on the work of Jacobsen, Fuhrman and Fredsøe (2012).

This repository is a **fork of [ogoe/waves2Foam](https://github.com/ogoe/waves2Foam)**, updated and verified to build and run with **OpenFOAM v2406** (OpenFOAM Foundation / OpenFOAM.org).

## What is included

| Component | Description |
|---|---|
| `src/waves2Foam` | Main library: wave theories (Stokes, cnoidal, stream function, irregular, solitary, ...), relaxation zones, wave generation / active absorption boundary conditions |
| `src/waves2FoamGABC` | Generalized ABC (GABC) boundary conditions (`gabcVelocity`, `gabcPressureRobinV`) |
| `src/waves2FoamProcessing` | Run-time post-processing: gauges, probes, raw data writers |
| `src/waves2FoamSampling2406` | Run-time surface elevation sampling for OpenFOAM v2406 (`sampledSurfaceElevation`, `overtopping`) |
| `src/waves2FoamPorosity` | Porous-media model (Jensen, Jacobsen & Christensen 2014) with resistance-coefficient calibration |
| `applications/solvers/solvers2206_PLUS`, `applications/solvers/solvers2406_PLUS` | Solvers: `waveFoam` (interFoam-based), `waveIsoFoam`, `porousWaveFoam`, `overWaveDyMFoam` (overset + dynamic mesh) |
| `applications/utilities/preProcessing` | Pre-processing tools: `setWaveField`, `setWaveParameters`, `waveGaugesNProbes`, `faceSetToSTL`, `relaxationZoneLayout`, `sampleIncidentWaveField` |
| `tutorials/` | Example cases for the solvers and relaxation-zone layout |
| `ThirdParty/` | Third-party dependencies: LAPACK 3.3.1, SPARSKIT2, OceanWave3D, fenton4Foam |

## Getting started

Clone the repository into your OpenFOAM user directory (the standard location for user applications):

```bash
mkdir -p $WM_PROJECT_USER_DIR/applications/utilities
cd $WM_PROJECT_USER_DIR/applications/utilities
git clone https://github.com/byChen47/waves2Foam.git
cd waves2Foam
```

You can actually clone it anywhere — `bin/bashrc` detects the repository root automatically, so `./Allwmake` works from any location.

## Requirements

* **OpenFOAM v2406** (OpenFOAM.org), built and sourced:
  ```bash
  source $HOME/OpenFOAM/<username>-v2406/etc/bashrc
  ```
  (The user directory is named after your OpenFOAM user, e.g. `chen-v2406` on the machine this fork was developed on. `$WM_PROJECT_USER_DIR` must point to it.)
* A C++ compiler (`gcc`/`g++`) and GNU `make`
* `gfortran` (Fortran compiler, required by the ThirdParty libraries)
* The GNU Scientific Library (GSL)
* [IHFoam](https://github.com/phicau/IHFOAM) — provides the `waveModels` library required by `waveFoam`

Install the required system packages (Ubuntu/Debian):
```bash
sudo apt update
sudo apt install build-essential cmake git subversion gfortran libgsl-dev
```

### GCC/Gfortran 9 (recommended)

waves2Foam is sensitive to the compiler version — the **GCC/Gfortran 9 series is recommended**. If your system default compiler is newer (e.g. Ubuntu 22.04 ships GCC 11 by default), install GCC 9 and switch to it:

```bash
# Install GCC 9
sudo apt install gcc-9 g++-9 gfortran-9

# Register GCC 9 as an alternative
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 100
sudo update-alternatives --install /usr/bin/gfortran gfortran /usr/bin/gfortran-9 100

# Switch the default to GCC 9
sudo update-alternatives --set gcc /usr/bin/gcc-9
sudo update-alternatives --set g++ /usr/bin/g++-9
sudo update-alternatives --set gfortran /usr/bin/gfortran-9
```

Verify the switch:
```bash
gcc --version
g++ --version
gfortran --version
```

> If your OpenFOAM installation itself was compiled with a different compiler version, it is safest to compile waves2Foam with the same compiler to avoid link/ABI problems.

## Build

```bash
cd $WM_PROJECT_USER_DIR/applications/utilities/waves2Foam
./Allwmake
```

### File permissions (Ubuntu)

`git clone` preserves the executable bits, so `./Allwmake` normally runs right away. If you downloaded the repository as a **ZIP archive** (GitHub's "Download ZIP"), the permissions are lost and you must restore them first:

```bash
find . -type f \( -name 'Allwmake' -o -name 'Allwclean' -o -name 'Allrun' -o -name 'prepareCase.sh' -o -name 'makeNewWaveTheory' \) -exec chmod +x {} +
```

Verify with:

```bash
ls -l Allwmake
```

It should show `-rwxr-xr-x`. The same command also works for filesystems that do not store Unix permissions (e.g. shared folders or some USB drives).

`./Allwmake` will:

1. **Set up the environment** by sourcing `bin/bashrc` (defines the `WAVES_*` variables, detects OpenFOAM v2406 and the OpenFOAM+ branch, and selects the matching `solvers2406_PLUS` / `waves2FoamSampling2406` sources).
2. **Build the ThirdParty dependencies** (`ThirdParty/Allwmake`):
   - LAPACK 3.3.1 and SPARSKIT2 (static libraries, built from the bundled tarballs)
   - OceanWave3D (executable + `libOceanWave3D.so`), used for external wave forcing
   - `fenton4Foam` (executable, for the `fenton` stream-function wave theory)
3. **Build the libraries**: `libwaves2Foam.so`, `libwaves2FoamGABC.so`, `libwaves2FoamProcessing.so`, `libwaves2FoamSampling.so`, `libwaves2FoamPorosity.so`.
4. **Build the solvers**: `waveFoam`, `waveIsoFoam`, `porousWaveFoam`, `overWaveDyMFoam`.
5. **Build the pre-processing utilities**: `setWaveField`, `setWaveParameters`, `waveGaugesNProbes`, `faceSetToSTL`, `relaxationZoneLayout`, `sampleIncidentWaveField`.

Built libraries and executables are installed into `$FOAM_USER_LIBBIN` / `$FOAM_USER_APPBIN` (by default `$HOME/OpenFOAM/<username>-v2406/platforms/linux64GccDPInt32Opt/lib` and `.../bin`).

### Notes

* **Always build through `./Allwmake`** (or source `bin/bashrc` first). The `WAVES_*` environment variables and the OpenFOAM version macros (`WM_PROJECT_VERSION_NUMBER`, `OFPLUSBRANCH`, ...) must be set; building individual components with `wmake` in a plain shell will silently produce broken compile flags.
* **OceanWave3D is bundled** in `ThirdParty/OceanWave3D-Fortran90` (source, license and README included), so a fresh clone builds everything without downloading extra sources. If you prefer a different version, replace that directory or set `OCEANWAVE3D_DIR` to the path of your own copy before running `./Allwmake`.
* If the build is interrupted, `./Allwmake` will pick up where it left off: already-compiled ThirdParty libraries and binaries are detected and skipped.

### OceanWave3D build fix (dash compatibility)

The bundled OceanWave3D makefile previously used a bash-only redirection (`&> /dev/null`) in its `*.mod` housekeeping steps. On systems where `/bin/sh` is **dash** (the default on Ubuntu/Debian), that construct is parsed differently, so the `then` branch always executed and an unconditional `cp` of `thirdpartylibs/LIB_VTK_IO/static/lib_vtk_io.mod` failed — that file does not exist in the bundled source tree (the only consumer, `StoreDataVTK.f90`, is commented out in `src/IO/makefile.inc`). The failing recipe aborted the OceanWave3D build early:

```text
mv: cannot stat '*.mod': No such file or directory
cp: cannot stat 'thirdpartylibs/LIB_VTK_IO/static/lib_vtk_io.mod': No such file or directory
make: *** [makefile:132: .../build/precision.o] Error 1
```

The missing `libOceanWave3D.so` then caused cascading `-lOceanWave3D` / `-lwaves2Foam` link errors.

The fix in `ThirdParty/settings/oceanWave3DSettings/makefile`:

* replaces `&> /dev/null` with the POSIX-compatible `> /dev/null 2>&1`, and
* only copies `lib_vtk_io.mod` when it actually exists.

`./Allwmake` copies this settings makefile over the bundled OceanWave3D source on every run, so fresh clones build out of the box on both bash and dash.

## overWaveDyMFoam

`overWaveDyMFoam` is a bundled solver that combines the **overset-mesh** and **dynamic-mesh** capabilities of OpenFOAM's `overInterDyMFoam` with the waves2Foam relaxation-zone wave generation and absorption framework. It is aimed at two-phase free-surface simulations on overlapping meshes, and supports VOF interface capturing, dynamic mesh motion, and external wave forcing.

It is provided for both OpenFOAM v2206 and v2406:

* `applications/solvers/solvers2206_PLUS/overWaveDyMFoam`
* `applications/solvers/solvers2406_PLUS/overWaveDyMFoam`

`./Allwmake` discovers and builds it automatically alongside the other solvers (no extra steps needed). See `applications/solvers/solvers2206_PLUS/overWaveDyMFoam/README.md` for a detailed description.

## Usage

1. Copy a tutorial case, e.g.
   ```bash
   cp -r $WAVES_TUT/waveFoam/waveFlume runCase
   cd runCase
   blockMesh
   setWaveParameters
   setWaveField
   waveFoam
   ```
2. See the individual `tutorials/*/Allrun` scripts for full automated setups.

## References

Please cite the following papers when using this toolbox:

> Jacobsen, N.G., Fuhrman, D.R. and Fredsøe, J. (2012). *A Wave Generation Toolbox for the Open-Source CFD Library: OpenFOAM®*. Int. J. for Numer. Meth. Fluids, 70(9), 1073–1088. DOI: [10.1002/fld.2726](https://doi.org/10.1002/fld.2726)

> Jensen, B., Jacobsen, N.G. and Christensen, E.D. (2014). *Investigations on the porous media equations and resistance coefficients for coastal structures*. Coastal Engineering, 84, 56–72. DOI: [10.1016/j.coastaleng.2013.11.002](https://doi.org/10.1016/j.coastaleng.2013.11.002)

## License

This project is released under the GNU General Public License. See the header of each source file for the full license text.

The wave generation and absorption framework was originally developed by **Niels G. Jacobsen** (Technical University of Denmark); the porosity module by **Bjarne Jensen** and **Niels G. Jacobsen**. See [CONTRIBUTORS](CONTRIBUTORS) for the full list of contributors.
