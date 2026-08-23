# waves2Foam

A toolbox for **volume-of-fluid (VOF) based wave generation and active wave absorption** in OpenFOAM, based on the work of Jacobsen, Fuhrman and Fredsøe (2012).

This repository is a **fork of [ogoe/waves2Foam](https://github.com/ogoe/waves2Foam)** that adds:

* **HOS (Higher-Order Spectral) wave generation** — HOS-NWT / HOS-ocean + Grid2Grid bridged into the waves2Foam relaxation zones (`HOSwaves2Foam/`), with regular- and irregular-wave tutorial cases;
* **`overWaveDyMFoam`** — an overset + dynamic-mesh wave solver;
* support for **OpenFOAM v2206 and v2406** (OpenFOAM+ branch), with all ThirdParty dependencies bundled so a fresh clone compiles out of the box.

## Repository structure

```text
waves2Foam/
├── Allwmake / Allwclean        # build / clean everything, including HOSwaves2Foam
├── bin/                        # environment (bin/bashrc) and case preparation scripts
├── src/
│   ├── waves2Foam/             # core library: wave theories, relaxation zones, wave BCs
│   ├── waves2FoamGABC/         # generalized ABC boundary conditions (gabcVelocity, ...)
│   ├── waves2FoamProcessing/   # run-time post-processing: gauges, probes, spectral analysis
│   ├── waves2FoamSampling2206/ # surface-elevation sampling (OpenFOAM v2206)
│   ├── waves2FoamSampling2406/ # surface-elevation sampling (OpenFOAM v2406)
│   └── waves2FoamPorosity/     # porous-media model
├── applications/
│   ├── solvers/solvers2206_PLUS/  # waveFoam, waveIsoFoam, porousWaveFoam, overWaveDyMFoam
│   ├── solvers/solvers2406_PLUS/  # same solvers for OpenFOAM v2406
│   └── utilities/                 # setWaveField, setWaveParameters, waveGaugesNProbes, ...
├── tutorials/                  # standard waves2Foam tutorial cases
├── ThirdParty/                 # LAPACK 3.3.1, SPARSKIT2, OceanWave3D, fenton4Foam
├── HOSwaves2Foam/              # HOS coupling module (see below)
│   ├── src/HOS-NWT/            #   vendored HOS-NWT source (numerical wave tank)
│   ├── src/HOS-ocean/          #   vendored HOS-ocean source (open ocean)
│   ├── src/Grid2Grid/          #   waves2Foam Grid2Grid coupling library
│   ├── ThirdParty/             #   vendored FFTW3 + LAPACK/BLAS build dependencies
│   ├── scripts/                #   buildHOS.sh, buildGrid2Grid.sh
│   ├── tutorials/              #   HOS-NWT, waveFoam-HOS, irregular, stokesI-HOS cases
│   └── doc/                    #   使用说明.md, HOS-Grid2Grid参数详解.md, ...
└── doc/                        # waves2Foam manual, source-code structure, memos
```

## Getting started

Clone the repository into your OpenFOAM user directory (the standard location for user applications):

```bash
mkdir -p $WM_PROJECT_USER_DIR/applications/utilities
cd $WM_PROJECT_USER_DIR/applications/utilities
git clone https://github.com/byChen47/waves2Foam.git
cd waves2Foam
```

You can actually clone it anywhere — `bin/bashrc` detects the repository root automatically, so `./Allwmake` works from any location. After cloning, go to [Build](#build); to run an example case, see [Usage](#usage).

## Requirements

* **OpenFOAM v2206 or v2406** (OpenFOAM.org), built and sourced:
  ```bash
  source $HOME/OpenFOAM/<username>-v<version>/etc/bashrc
  ```
  (`$WM_PROJECT_USER_DIR` must point to your OpenFOAM user directory, e.g. `chen-v2206`.)
* A C++ compiler (`gcc`/`g++`) and GNU `make`
* `gfortran` (Fortran compiler, required by the ThirdParty libraries and the HOS models)
* `cmake` (required by the HOS models; the vendored FFTW3/LAPACK need no system install)
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

### What `./Allwmake` does

1. **Sets up the environment** by sourcing `bin/bashrc` — defines the `WAVES_*` variables, detects the installed OpenFOAM version and the OpenFOAM+ branch, and selects the matching `solvers<version>_PLUS` / `waves2FoamSampling<version>` sources.
2. **Builds the ThirdParty dependencies** (`ThirdParty/Allwmake`):
   - LAPACK 3.3.1 and SPARSKIT2 (static libraries, built from the bundled tarballs)
   - OceanWave3D (executable + `libOceanWave3D.so`), used for external wave forcing
   - `fenton4Foam` (executable, for the `fenton` stream-function wave theory)
3. **Builds the libraries**: `libwaves2Foam.so`, `libwaves2FoamGABC.so`, `libwaves2FoamProcessing.so`, `libwaves2FoamSampling.so`, `libwaves2FoamPorosity.so`.
4. **Builds the solvers**: `waveFoam`, `waveIsoFoam`, `porousWaveFoam`, `overWaveDyMFoam`.
5. **Builds the pre-processing utilities**: `setWaveField`, `setWaveParameters`, `waveGaugesNProbes`, `faceSetToSTL`, `relaxationZoneLayout`, `sampleIncidentWaveField`.
6. **Builds the HOSwaves2Foam coupling**: HOS-NWT + HOS-ocean (`HOSwaves2Foam/scripts/buildHOS.sh`), Grid2Grid (`scripts/buildGrid2Grid.sh`) and `libwaves2FoamGrid2Grid.so` (`wmake`). Already-built HOS binaries / installed libraries are detected and skipped.

Built libraries and executables are installed into `$FOAM_USER_LIBBIN` / `$FOAM_USER_APPBIN` (by default `$HOME/OpenFOAM/<username>-v<version>/platforms/linux64GccDPInt32Opt/lib` and `.../bin`). `./Allwclean` cleans everything, including the HOS build artifacts and coupling libraries.

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

### Notes

* **Always build through `./Allwmake`** (or source `bin/bashrc` first). The `WAVES_*` environment variables and the OpenFOAM version macros (`WM_PROJECT_VERSION_NUMBER`, `OFPLUSBRANCH`, ...) must be set; building individual components with `wmake` in a plain shell will silently produce broken compile flags.
* **OceanWave3D is bundled** in `ThirdParty/OceanWave3D-Fortran90` (source, license and README included), so a fresh clone builds everything without downloading extra sources. If you prefer a different version, replace that directory or set `OCEANWAVE3D_DIR` to the path of your own copy before running `./Allwmake`.
* If the build is interrupted, `./Allwmake` will pick up where it left off: already-compiled ThirdParty libraries and binaries are detected and skipped.

## Solvers

Four solvers are provided, each under `applications/solvers/solvers2206_PLUS/` and `applications/solvers/solvers2406_PLUS/`:

| Solver | Description |
|---|---|
| `waveFoam` | InterFoam-based solver with waves2Foam wave generation / absorption boundary conditions |
| `waveIsoFoam` | IsoAdvector-based variant of `waveFoam` |
| `porousWaveFoam` | `waveFoam` extended with the porous-media model |
| `overWaveDyMFoam` | Overset + dynamic mesh wave solver (see below) |

### overWaveDyMFoam

`overWaveDyMFoam` combines the **overset-mesh** and **dynamic-mesh** capabilities of OpenFOAM's `overInterDyMFoam` with the waves2Foam relaxation-zone wave generation and absorption framework. It is aimed at two-phase free-surface simulations on overlapping meshes, and supports VOF interface capturing, dynamic mesh motion, and external wave forcing.

* Source: `applications/solvers/solvers2206_PLUS/overWaveDyMFoam` and `applications/solvers/solvers2406_PLUS/overWaveDyMFoam`
* Detailed description: `applications/solvers/solvers2206_PLUS/overWaveDyMFoam/README.md`

`./Allwmake` discovers and builds it automatically alongside the other solvers — no extra steps are needed.

## HOS wave coupling (HOSwaves2Foam)

`HOSwaves2Foam/` implements wave generation with **Higher-Order Spectral (HOS)**
potential-flow models (HOS-NWT / HOS-ocean, LHEEA-ECN), bridged to the CFD
domain through **Grid2Grid** and the waves2Foam relaxation zones:

```text
HOS-NWT / HOS-ocean  ->  Grid2Grid (libGrid2Grid.so)
                    ->  libwaves2FoamGrid2Grid.so  ->  waveFoam relaxation zone
```

The HOS models and all build dependencies (FFTW3, LAPACK/BLAS) are vendored,
so everything compiles with two commands (and is also built automatically by
`./Allwmake`, step 6 above):

```bash
bash HOSwaves2Foam/scripts/buildHOS.sh          # HOS-NWT + HOS-ocean
bash HOSwaves2Foam/scripts/buildGrid2Grid.sh    # libGrid2Grid.so -> $FOAM_USER_LIBBIN
cd HOSwaves2Foam/src/Grid2Grid && wmake libso   # libwaves2FoamGrid2Grid.so
```

Tutorial cases (each already contains its HOS-NWT SWENSE mode file and runs
out of the box with `Allrun`):

| Case | Waves |
|---|---|
| `HOSwaves2Foam/tutorials/waveFoam-HOS` | Regular wave (A = 0.05 m, T = 2 s) |
| `HOSwaves2Foam/tutorials/waveFoam-HOS-Irregular` | Irregular waves (JONSWAP, Hs = 0.05 m, Tp = 1 s) |
| `HOSwaves2Foam/tutorials/stokesI-HOS` | Large 624 m tank, Stokes-I-equivalent regular wave (H = 0.05 m, T = 3 s), converted from the original `StokesI` (IHFoam) setup to HOS |
| `HOSwaves2Foam/tutorials/stokesI-HOS-3D` | 3D oblique-wave version of stokesI-HOS (20 m wide, 15 deg, n2 = 8) |
| `HOSwaves2Foam/tutorials/HOS-NWT` / `HOS-NWT-Irregular` | HOS-NWT generator cases for the regular / irregular waves above |

Documentation (Chinese):

* [`HOSwaves2Foam/doc/使用说明.md`](HOSwaves2Foam/doc/使用说明.md) — complete usage guide, line-by-line configuration explanation and wave-field setup;
* [`HOSwaves2Foam/doc/HOS-Grid2Grid参数详解.md`](HOSwaves2Foam/doc/HOS-Grid2Grid参数详解.md) — full Grid2Grid parameter reference;
* each tutorial case has its own `README.md`.

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
