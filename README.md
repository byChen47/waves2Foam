# waves2Foam

A toolbox for **volume-of-fluid (VOF) based wave generation and active wave absorption** in OpenFOAM, based on the work of Jacobsen, Fuhrman and Fredsøe (2012).

This repository is a **fork of [ogoe/waves2Foam](https://github.com/ogoe/waves2Foam)** that adds:

* **HOS (Higher-Order Spectral) wave generation** — HOS-NWT / HOS-ocean + Grid2Grid bridged into the waves2Foam relaxation zones (`HOSwaves2Foam/`), with regular- and irregular-wave tutorial cases;
* **`overWaveDyMFoam`** — an overset + dynamic-mesh wave solver;
* **`waveMonitoring/`** — a standalone wave-measurement module (free-surface elevation function object + spectral analysis) that works with *any* VOF solver and links nothing from waves2Foam;
* support for **OpenFOAM v2206, v2406 and v2606** (OpenFOAM+ branch), with all ThirdParty dependencies bundled so a fresh clone compiles out of the box. Shared sources are version-conditional, so one tree still builds on the older branches — see [Supported OpenFOAM versions](#supported-openfoam-versions).

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
│   ├── waves2FoamSampling2606/ # surface-elevation sampling (OpenFOAM v2606)
│   └── waves2FoamPorosity/     # porous-media model
├── applications/
│   ├── solvers/solvers2206_PLUS/  # waveFoam, waveIsoFoam, porousWaveFoam, overWaveDyMFoam
│   ├── solvers/solvers2406_PLUS/  # same solvers for OpenFOAM v2406
│   ├── solvers/solvers2606_PLUS/  # same solvers for OpenFOAM v2606
│   └── utilities/                 # setWaveField, setWaveParameters, waveGaugesNProbes, ...
├── waveMonitoring/             # standalone wave measurement (any solver)
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

* **OpenFOAM v2206, v2406 or v2606** (OpenCFD/ESI releases), built and sourced:
  ```bash
  source $HOME/OpenFOAM/OpenFOAM-v<version>/etc/bashrc
  ```
  `$WM_PROJECT_USER_DIR` then points to your OpenFOAM user directory
  (e.g. `$HOME/OpenFOAM/<username>-v2606`), which is where waves2Foam is expected to live.
* A C++ compiler (`gcc`/`g++`) and GNU `make`; for **v2606** it must support
  C++17 (OpenFOAM's wmake rules add `-std=c++17` automatically — this fork is
  built and tested with GCC 9.5)
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

No OpenFOAM environment needs to be loaded first: if none is detected,
`./Allwmake` sources `~/OpenFOAM/OpenFOAM-*/etc/bashrc` itself. When several
OpenFOAM versions are installed side by side, pin the one you want to build
against:

```bash
OF_BASHRC=$HOME/OpenFOAM/OpenFOAM-v2406/etc/bashrc ./Allwmake
```

### What `./Allwmake` does

1. **Sets up the environment** — sources an OpenFOAM installation if none is loaded
   (honouring `$OF_BASHRC`), then `bin/bashrc`, which defines the `WAVES_*`
   variables, detects the installed OpenFOAM version and branch, and selects the
   matching `solvers<version>_PLUS` and `waves2FoamSampling<version>` sources.
2. **Builds the ThirdParty dependencies** (`ThirdParty/Allwmake`):
   - LAPACK 3.3.1 and SPARSKIT2 (static libraries, built from the bundled tarballs)
   - OceanWave3D (executable + `libOceanWave3D.so`), used for external wave forcing
   - `fenton4Foam` (executable, for the `fenton` stream-function wave theory)
3. **Builds the libraries**: `libwaves2Foam.so`, `libwaves2FoamGABC.so`, `libwaves2FoamProcessing.so`, `libwaves2FoamSampling.so`, `libwaves2FoamPorosity.so`.
4. **Builds the solvers**: `waveFoam`, `waveIsoFoam`, `porousWaveFoam`, `overWaveDyMFoam`.
5. **Builds the pre-processing utilities**: `setWaveField`, `setWaveParameters`, `waveGaugesNProbes`, `faceSetToSTL`, `relaxationZoneLayout`, `sampleIncidentWaveField`.
6. **Builds the HOSwaves2Foam coupling**: HOS-NWT + HOS-ocean (`HOSwaves2Foam/scripts/buildHOS.sh`), Grid2Grid (`scripts/buildGrid2Grid.sh`) and `libwaves2FoamGrid2Grid.so` (`wmake`). Already-built HOS binaries / installed libraries are detected and skipped.

Libraries, the pre-processing utilities and `porousWaveFoam` / `waveIsoFoam` are installed into
`$FOAM_USER_LIBBIN` / `$FOAM_USER_APPBIN` (by default
`$HOME/OpenFOAM/<username>-v<version>/platforms/linux64GccDPInt32Opt/lib` and `.../bin`).
`waveFoam` and `overWaveDyMFoam` are installed into `$FOAM_APPBIN`, i.e. inside the
OpenFOAM installation itself; if OpenFOAM was installed system-wide and that
directory is not writable, change `EXE` in their `Make/files` to
`$(WAVES_APPBIN)` instead. `./Allwclean` cleans everything, including the HOS
build artifacts and coupling libraries.

`./Allwmake` reports failures: every library, solver and utility is checked, a
failing step prints `*** ... FAILED`, and the script exits non-zero instead of
printing `COMPILATION DONE`.

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

## Supported OpenFOAM versions

`bin/bashrc` and `src/Allwmake` select the sources from the OpenFOAM version
number, so the same tree serves all supported branches:

| OpenFOAM | Solver sources | Sampling sources |
|---|---|---|
| **v2606** | `applications/solvers/solvers2606_PLUS/` | `src/waves2FoamSampling2606/` |
| **v2406** | `applications/solvers/solvers2406_PLUS/` | `src/waves2FoamSampling2406/` |
| **v2206** | `applications/solvers/solvers2206_PLUS/` | `src/waves2FoamSampling2206/` |
| v1712 – v2012 | `solvers1712_PLUS/` … `solvers2012_PLUS/` | `src/waves2FoamSampling1712/` |
| < v1706 | `solvers<version>_PLUS/` | `src/waves2FoamSampling/` |

`src/Allwmake` maps the version number to the sampling directory as
`<1706 → waves2FoamSampling`, `<2206 → …1712`, `<2406 → …2206`,
`<2606 → …2406`, otherwise `…2606`.

Code shared by all versions uses compile-time switches where the OpenFOAM API
changed, so upgrading OpenFOAM does not require editing the sources:

| API | Handling |
|---|---|
| `directionMixedFvPatchField` copy constructor | deleted from **v2506**; `gabcVelocity` uses `directionMixedFvPatchVectorField(pfld, pfld.internalField())` under `#if OFPLUSBRANCH == 1 && 2506 <= OFVERSION` and the previous form otherwise |
| `globalIndex::reset()` | tag-first argument order from **v2506**; only `waves2FoamSampling2606/` uses the new form |
| `interpolatedFaces.H` | removed in **v2506**; `overWaveDyMFoam` includes it only on older versions and raises a fatal error if `massFluxInterpolation` is enabled on newer ones |

To add support for a future release (e.g. v2612), copy
`applications/solvers/solvers2606_PLUS/` and `src/waves2FoamSampling2606/` to the
new version number; extend the version `if` chains in `bin/bashrc` and
`src/Allwmake` only if the mapping changes. This fork is currently built and
tested with **v2606**; the v2206/v2406 trees are kept unchanged.

## IDE setup (VS Code)

VS Code does not recognise OpenFOAM's uppercase `.C`/`.H` files or its `lnInclude` header layout out of the box, so IntelliSense reports `#include errors detected` and code you wrote earlier is not auto-completed. The complete, step-by-step fix (global user settings, `c_cpp_properties.json`, full `lnInclude` path list, preprocessor defines and `wmake` build tasks) is documented in [OpenFOAMPathSettingReadme.md](OpenFOAMPathSettingReadme.md).

## Solvers

Four solvers are provided, per OpenFOAM version under
`applications/solvers/solvers<version>_PLUS/` (`solvers2206_PLUS/`,
`solvers2406_PLUS/`, `solvers2606_PLUS/`):

| Solver | Description |
|---|---|
| `waveFoam` | InterFoam-based solver with waves2Foam wave generation / absorption boundary conditions |
| `waveIsoFoam` | IsoAdvector-based variant of `waveFoam` |
| `porousWaveFoam` | `waveFoam` extended with the porous-media model |
| `overWaveDyMFoam` | Overset + dynamic mesh wave solver (see below) |

### overWaveDyMFoam

`overWaveDyMFoam` combines the **overset-mesh** and **dynamic-mesh** capabilities of OpenFOAM's `overInterDyMFoam` with the waves2Foam relaxation-zone wave generation and absorption framework. It is aimed at two-phase free-surface simulations on overlapping meshes, and supports VOF interface capturing, dynamic mesh motion, and external wave forcing.

* Source: `overWaveDyMFoam/` in `applications/solvers/solvers2206_PLUS/`, `solvers2406_PLUS/` and `solvers2606_PLUS/`
* Detailed description: `applications/solvers/solvers2206_PLUS/overWaveDyMFoam/README.md`
* On OpenFOAM **v2506 and newer** the `massFluxInterpolation` switch in `fvSolution`
  is no longer available (upstream removed `interpolatedFaces.H`). It defaults to
  `false`; enabling it makes the solver stop with an explicit fatal error.

`./Allwmake` discovers and builds it automatically alongside the other solvers — no extra steps are needed.

## Wave monitoring (standalone)

`waveMonitoring/` is a self-contained module for measuring waves with **any**
VOF solver - no waves2Foam solver or library involved:

* `libwaveMonitoring.so` provides the `waveElevation` function object: it writes
  the free-surface elevation time series at a list of gauges to a single
  self-describing file (`postProcessing/waveElevation/<name>/elevation.dat`);
* `waveSpectra` is a post-processing utility that turns that file into spectra,
  `Hm0`, `Tp`, `Tm02`, `H1/3`, `T1/3`, `Hmax` and mean water level.

```bash
cd waveMonitoring && ./Allwmake          # needs OpenFOAM only

# then, in any case's system/controlDict:
functions
{
    waveElevation1
    {
        type     waveElevation;
        libs     ("libwaveMonitoring.so");
        locations ((0.10 0.05 0.005) (0.25 0.05 0.005) (0.40 0.05 0.005));
        writeControl timeStep;
        writeInterval 1;
    }
}

# after the run:
waveSpectra -file postProcessing/waveElevation/waveElevation1/elevation.dat \
            -out waveSpectra -tStart 0.6
```

The elevation is the *integrated* liquid height along the vertical ray through
each gauge (`eta = integral(alpha dl)`), which stays well behaved for aerated or
broken surfaces, and equals the free-surface height above the bottom in a
flat-bottom tank. See [`waveMonitoring/README.md`](waveMonitoring/README.md) for
the full parameter list, output format, algorithm and validation, and
[`waveMonitoring/examples/`](waveMonitoring/examples) for a 5-minute damBreak
smoke test.

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

## Troubleshooting

`./Allwmake` stops at the first failing component and names it, for example:

```text
*** FAILED: library waves2FoamGABC
*** LIBRARY BUILD FAILED (exit 1) - see the log above ***
```

| Symptom | Cause / fix |
|---|---|
| `use of deleted function ...directionMixedFvPatchField(const this_bctype&)` | Building v2506+ with a source tree that lacks the guarded copy constructor (see [Supported OpenFOAM versions](#supported-openfoam-versions)). |
| `no matching function for call to 'Foam::globalIndex::reset(Foam::label, ...)'` | v2506+ made the dispatch tag the first argument; use `waves2FoamSampling2606/` (or newer) rather than an older sampling directory. |
| `fatal error: interpolatedFaces.H: No such file or directory` | That header was removed in v2506; see the `overWaveDyMFoam` note above. |
| `cannot find -lwaves2Foam...` | The library did not build; run `wmake libso` in its `src/...` directory and read the first `error:`. |
| `wmkdepend: could not open 'foamTime.H'` | Harmless dependency-scan message on newer OpenFOAM. |
| `WM_PROJECT_DIR: unbound variable` while sourcing | The shell runs with `set -u`; source the OpenFOAM `etc/bashrc` without it. |

## References

Please cite the following papers when using this toolbox:

> Jacobsen, N.G., Fuhrman, D.R. and Fredsøe, J. (2012). *A Wave Generation Toolbox for the Open-Source CFD Library: OpenFOAM®*. Int. J. for Numer. Meth. Fluids, 70(9), 1073–1088. DOI: [10.1002/fld.2726](https://doi.org/10.1002/fld.2726)

> Jensen, B., Jacobsen, N.G. and Christensen, E.D. (2014). *Investigations on the porous media equations and resistance coefficients for coastal structures*. Coastal Engineering, 84, 56–72. DOI: [10.1016/j.coastaleng.2013.11.002](https://doi.org/10.1016/j.coastaleng.2013.11.002)

## License

This project is released under the GNU General Public License. See the header of each source file for the full license text.

The wave generation and absorption framework was originally developed by **Niels G. Jacobsen** (Technical University of Denmark); the porosity module by **Bjarne Jensen** and **Niels G. Jacobsen**. See [CONTRIBUTORS](CONTRIBUTORS) for the full list of contributors.
